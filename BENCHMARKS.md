# Benchmarks & Profiling

Notes from actually measuring the `/recommend` query path instead of assuming it's fast: a benchmark harness, a compiler flag sweep (including an automated random/evolutionary search), `gprof`/`perf` profiling, two optimizations with before/after numbers, and an accuracy check on the MinHash signal. Everything here is reproducible from `bench/`.

## Running it yourself

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON
cmake --build build --target bench_runner minhash_eval
./bench_runner      # p50/p99 latency + the optimization's correctness check
./minhash_eval       # MinHash vs. Jaccard recall@k + speed
ctest --test-dir build   # same correctness check, wired into CTest
python3 bench/flag_search.py  # automated compiler-flag search, see below (run under WSL/Linux)
```

Run the CMake targets from the repo root — they look for `data/` the same way the server does. `BUILD_BENCHMARKS` defaults off, so a normal build (and the Docker image) never pulls this in.

## Compiler flags

| Build | `/recommend/global` scan, p50 |
|---|---|
| Debug (`-O0`) | ~158 ms |
| `-O2` | ~22-29 ms |
| `-O3` (Release) | ~22-31 ms |
| `-O3 -march=native` | ~20-31 ms |

The Dockerfile already builds with `-DCMAKE_BUILD_TYPE=Release`, so production was never running unoptimized. The real gap is local dev builds: CLion's default profile is Debug, which measured **~5-6x slower** than any optimized build. `-O2` vs `-O3` vs `-march=native` are statistically indistinguishable for this code — not surprising, since it's simple loops over fixed-size arrays rather than anything branch-heavy. (Ranges reflect real run-to-run variance on a dev laptop doing other things, not measurement error.)

## gcc vs. clang

Run on the same machine (WSL Ubuntu, so both compilers share an OS/libc - the Windows numbers above use MinGW GCC and aren't directly comparable) at matched flags: GCC 15.2.0 vs. Clang 21.1.8, both `-O3 -march=native`, no `-ffast-math`.

This isn't just a one-off local comparison: CI (`.github/workflows/ci.yml`, `backend-build-clang`) configures and builds `steam_server`/`data_converter`/`bench_runner` with Clang and runs the correctness test against it on every push and PR, alongside the existing GCC/Docker build. The project actually compiles clean under both compilers with zero source changes required - this is enforced going forward, not asserted once.

| Function | gcc, p50 | clang, p50 |
|---|---|---|
| `getJaccard` | ~21-26 ns | ~20-34 ns |
| `getMinHash` | ~65-85 ns | ~67-78 ns |
| `getCosine` | ~74-88 ns | **~150-190 ns** |

`getJaccard` and `getMinHash` land in the same ballpark under both compilers. `getCosine` doesn't — clang is consistently **~2x slower**, reproduced across three runs each.

**The actual mechanism, verified by reading the generated assembly, not just the compiler's remark text** (`getCosine`'s 128-float dot product, `Similarity.h:33`):

Clang's `-Rpass-missed=loop-vectorize` remark on this loop:

```
remark: loop not vectorized: cannot prove it is safe to reorder floating-point
operations; allow reordering by specifying '#pragma clang loop vectorize(enable)'
before the loop or by providing the compiler option '-ffast-math'
```

That remark is accurate for what Clang actually generates: a **fully scalar** loop, 16x unrolled, using `vfmadd132ss`/`vfmadd231ss` (fused multiply-add on one float at a time), with each accumulation strictly dependent on the previous one — no SIMD lanes at all. Clang evidently only considered a vectorization strategy that would need to reorder the summation itself, declined it without permission, and fell back to scalar FMA rather than finding a safe alternative.

GCC's assembly for the identical line does something smarter: it vectorizes the **multiplications** into 8-wide `vmulps`, but then unpacks each of those 8 products one at a time (`vshufps`/`vunpckhps`/`vextractf128`) and adds them into the accumulator via a strict, serial chain of scalar `vaddss` instructions, in the exact same left-to-right order the scalar source would. **This never reorders the sum at all** - it's a different optimization (batch the independent multiplications, keep the dependent reduction serial), not a riskier one. Proof, not inference: a build with `-fno-tree-vectorize` (no vectorization whatsoever) produces a **bit-for-bit identical result** to the default `-O3 -march=native` build (`0x1.e585fcp-3` both times, for the same two real games from the dataset) - and that multiply-only vectorization is a real, free ~30% per-call speedup (81.66ns vs. 116.28ns, fully scalar) for zero precision cost. Only `-ffast-math` actually changes the computed value (`0x1.e585fap-3` - different in the last representable bit), confirming reassociation only happens once something is actually told to reorder.

So the earlier framing in this doc - "gcc defaulting to a faster-but-technically-reorders-your-floats answer" - was wrong, and is corrected here: **GCC doesn't reorder anything by default on this loop.** It found a vectorization that never needed to. The honest comparison is: GCC's vectorizer found a safe partial-vectorization strategy for this reduction; Clang's, for whatever reason, didn't consider or apply that same strategy here, and only offered an all-or-nothing choice between "fully scalar" and "fully reordered, needs `-ffast-math`." Rebuilding with `-ffast-math` on Clang does close the gap - `getCosine` drops to ~60-87ns, matching or beating gcc, and the full scan drops from ~28-32ms to ~16-17ms - but that's Clang catching up via a different (full-reorder) mechanism than the one GCC uses by default, not Clang becoming "as safe as GCC already was." The optimized-vs-reference correctness test still passes under Clang's `-ffast-math`, since both paths being compared are compiled under the same flags within the same binary either way.

This isn't "clang is worse," either - both compilers are IEEE-754-safe by default here; GCC's vectorizer is just better at finding a vectorization strategy that doesn't require breaking that safety. `-ffast-math` is a real tradeoff (no safe NaN/Inf handling, no signed zeros, reassociation everywhere in the translation unit, not just this loop) and shouldn't be reached for by default - it's one legitimate way to close Clang's gap on this specific loop once you've confirmed what it's buying you (which is exactly what measuring this showed), but it's solving a problem GCC's default codegen didn't have in the first place.

Neither compiler vectorizes `getJaccard`'s popcount loop (`Similarity.h:15`, 8 iterations) - gcc's `-fopt-info-vec-all` reports a failed vectorization attempt; clang doesn't even emit a remark for it, consistent with the earlier call to leave it alone given it's only ~5% of scan time either way.

## Automated flag search (random + evolutionary)

`bench/flag_search.py` automates the compiler-flag question instead of hand-sweeping it: it tries combinations of `-O2`/`-O3`, `-march=native`, LTO, and loop-unrolling flags, builds and runs the real benchmark for each, and keeps whichever one is fastest on the production-representative row (`recommend/global`, 4 threads) - but only among combos that still pass bench_runner's correctness check. A faster build that changes the output gets logged as rejected, not kept.

```bash
wsl -- python3 bench/flag_search.py --seed 42
```

Needs a toolchain with LTO support to try that axis meaningfully - CLion's bundled MinGW GCC is built with `--disable-lto` (confirmed via `g++ -v`), so this runs under WSL's GCC instead, invoking `g++` directly on `bench_main.cpp`/`GameStore.cpp` rather than going through CMake.

Two phases:
1. **Random search** — samples flag combinations uniformly at random (with repeats) from the 24-combination space and evaluates each.
2. **Evolutionary search** — seeds a small population with the best combos random search found, then for a few generations mutates each (flips exactly one flag axis) and keeps the best of parents+children. With only 24 possible combinations this is really mutation-based local search, not a GA with room to discover something random sampling couldn't - included because it's what was asked for, not because the search space needs it.

Ran twice (same `--seed 42`, so the identical sequence of combinations was tried both times - only the measured timings differ run to run):

| Run | Winner | p50 |
|---|---|---|
| 1 | `-O3 -march=native` | 6127us |
| 2 | `-O3 -march=native -fno-unroll-loops` | 5662us |

The exact winner moved between runs (expected - these are single-digit-millisecond measurements on a shared machine, see the noise caveat at the bottom of this doc), but the pattern underneath didn't: in both runs, every combination in the top 5 included `-march=native`, and every combination with `-flto` or an explicit `-funroll-loops`/`-fno-unroll-loops` scored at or below the `-march=native`-only baseline - never clearly above it. Zero correctness rejections across either run (neither axis touches floating-point semantics, unlike the `-ffast-math` case above). The honest read: for this workload, `-march=native` is the one flag of the four that matters; LTO and explicit unroll hints are noise at best here, not free wins, which matches the earlier compiler-flag table's finding that `-O2`/`-O3`/`-march=native` were close but also explains why reaching further (LTO, unroll pragmas) doesn't pay off without a different bottleneck to target.

## Profiling

`gprof` (no elevated permissions needed) and later real `perf` with call graphs (`perf record -g -e task-clock`, since WSL2 doesn't expose hardware PMU counters) both agree on where the time in a `/recommend/global` scan goes:

- **~55-60%** — the three similarity functions themselves (`getCosine`'s 128-float dot product is the largest single piece)
- **~15-18%** — `std::sort` sorting the full candidate list just to take the top 90
- **~2-3%** — thread dispatch (after threading was added, see below)
- the rest is loop/iterator overhead

GCC's `-fopt-info-vec-all` confirms `getCosine` and `getMinHash`'s loops already auto-vectorize to AVX2 (32-byte vectors) at `-O3 -march=native`. `getJaccard`'s `__builtin_popcount` loop does not vectorize in any mode GCC tried — but it's only ~5% of scan time, so left alone.

## Optimization: bounded top-k + threaded scan

The `0.15`/`0.1` score thresholds on `/recommend` barely filter the dataset — tens of thousands of the 111,452 games clear them on a typical query — so fully sorting that candidate list just to take the top 90 was mostly wasted work.

- **Bounded top-k heap** (`src/TopK.h`) replaces the full `std::sort` with a size-capped max-heap: ~20-25% faster on its own.
- **Threaded scan** (`src/ParallelTopK.h`) splits the scan across worker threads, each keeping its own local top-90, merged at the end. Thread count is capped at `min(4, hardware_concurrency())` rather than hardcoded, so a small/constrained host falls back to the original single-threaded path (zero extra threads spawned) instead of over-subscribing cores it doesn't have.
- **Combined: ~2-3x faster** than the original full-sort, measured via `bench_runner`.

Correctness is checked, not assumed: `ctest`'s `recommend_topk_matches_reference` compares the optimized path's output against a brute-force reference on every build. It caught a real bug during development — the same scoring formula, written out separately in three places, rounded differently under `-march=native`'s FMA fusion, producing last-bit-different floats between the "optimized" and "reference" paths. Fixed by factoring it into one shared `getGlobalWeighted()` in `src/Similarity.h`.

## MinHash accuracy vs. the Jaccard signal

**Caveat first:** `getJaccard()` is exact, but only over the ~256 of 453 total tags that fit in `CompactGame::tagBits` (first 256 by `tags.txt` order) — not a game's full tag set. `getMinHash()` has no such cap. So this isn't "MinHash vs. true exact Jaccard" (the original `games.json` this was converted from is no longer available to compute that); it's MinHash's full-tag-set estimate checked against the existing truncated Jaccard already used elsewhere in the app. Run `./minhash_eval` to reproduce.

| Metric | Result |
|---|---|
| recall@10 | 22.1% |
| recall@50 | 27.6% |
| recall@90 | 29.8% |
| `getJaccard` per call | 28ns (p50) |
| `getMinHash` per call | 87.5ns (p50) |

Two findings here, and the second one is the more interesting:

1. **MinHash's top-k only agrees with Jaccard's top-k 22-30% of the time.** Some of that is structural (they're not estimating the same ground truth, per the caveat above), not pure MinHash error — but it's still a low number, and not something the code currently surfaces or claims.
2. **MinHash is ~3x *slower* per call, not faster.** The usual case for MinHash is trading accuracy for speed. That doesn't hold here, because the "exact" Jaccard already uses a cheap 256-bit popcount representation (8 word-level ops) that's cheaper than scanning a 150-element MinHash signature. So as implemented, MinHash is paying a speed cost without a clear accuracy win.

Side finding from building this eval: 659 games have real tags that are entirely invisible to `getJaccard()` because none of them landed in the first 256 slots of `tags.txt`. Small population (0.6% of the catalog), but a real, silent gap in the 256-bit truncation independent of anything about MinHash.

**Not acted on, just measured:** whether to widen `tagBits` to cover all 453 tags, and whether to keep MinHash's 0.3 weight in the `/recommend/global` blend given the above, are ranking-quality decisions rather than performance ones.

## What's not covered here

- **AVX2 intrinsics / struct-of-arrays layout for `CompactGame`** — the profiling data doesn't support the effort; the dominant costs are already auto-vectorized or are algorithmic (the sort/threading work above), not layout-bound.
- **An actual flamegraph SVG** — `perf` data was captured, but rendering it needed running third-party scripts the local sandbox didn't allow executing unreviewed; the text-based `perf report` breakdown above carries the same information.

All numbers above are from one dev machine and vary run-to-run (observed up to ~30-40% swings on repeated runs of the identical binary) — treat them as directional, not as guaranteed throughput on any other host.

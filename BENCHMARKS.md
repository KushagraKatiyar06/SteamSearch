# Benchmarks & Profiling

Notes from actually measuring the `/recommend` query path instead of assuming it's fast: a benchmark harness, a compiler flag sweep, `gprof`/`perf` profiling, two optimizations with before/after numbers, and an accuracy check on the MinHash signal. Everything here is reproducible from `bench/`.

## Running it yourself

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=ON
cmake --build build --target bench_runner minhash_eval
./bench_runner      # p50/p99 latency + the optimization's correctness check
./minhash_eval       # MinHash vs. Jaccard recall@k + speed
ctest --test-dir build   # same correctness check, wired into CTest
```

Run both from the repo root — they look for `data/` the same way the server does. `BUILD_BENCHMARKS` defaults off, so a normal build (and the Docker image) never pulls this in.

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

| Function | gcc, p50 | clang, p50 |
|---|---|---|
| `getJaccard` | ~21-26 ns | ~20-34 ns |
| `getMinHash` | ~65-85 ns | ~67-78 ns |
| `getCosine` | ~74-88 ns | **~150-190 ns** |

`getJaccard` and `getMinHash` land in the same ballpark under both compilers. `getCosine` doesn't — clang is consistently **~2x slower**, reproduced across three runs each.

The cause, from clang's own vectorization remarks (`-Rpass=loop-vectorize -Rpass-missed=loop-vectorize`) on `getCosine`'s 128-float dot product (`Similarity.h:33`):

```
remark: loop not vectorized: cannot prove it is safe to reorder floating-point
operations; allow reordering by specifying '#pragma clang loop vectorize(enable)'
before the loop or by providing the compiler option '-ffast-math'
```

Floating-point addition isn't associative, so vectorizing a `dot +=` reduction changes which roundings happen in which order. GCC's `-O3` vectorizes this reduction anyway (confirmed earlier via `-fopt-info-vec-all`); clang's `-O3` refuses without being told reordering is acceptable. Rebuilding with `-ffast-math` makes clang vectorize the same loop and closes the gap - `getCosine` drops to ~60-87 ns, matching or beating gcc, and the full `/recommend/global` scan drops from ~28-32 ms to ~16-17 ms (back in line with gcc's ~16.5-18 ms). The optimized-vs-reference correctness test still passes under `-ffast-math`, since both paths in that comparison are compiled under the same flags within the same binary.

This isn't "clang is worse" - it's clang defaulting to the IEEE-754-safe answer and gcc defaulting to a faster-but-technically-reorders-your-floats answer, for the exact same source line. `-ffast-math` is a real tradeoff (no safe NaN/Inf handling, no signed zeros, reassociation everywhere in the translation unit, not just this loop) and shouldn't be reached for by default - but for a hot reduction loop like this one, specifically allowing reassociation (`-fassociative-math -fno-signed-zeros -freciprocal-math`, the subset of `-ffast-math` that actually matters here) is a reasonable, scoped choice once you've confirmed what it's buying you - which is exactly what measuring this showed.

Neither compiler vectorizes `getJaccard`'s popcount loop (`Similarity.h:15`, 8 iterations) - gcc's `-fopt-info-vec-all` reports a failed vectorization attempt; clang doesn't even emit a remark for it, consistent with the earlier call to leave it alone given it's only ~5% of scan time either way.

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

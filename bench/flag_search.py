#!/usr/bin/env python3
"""Random + evolutionary search over compiler flags for the /recommend scan.

Tries combinations of -O2/-O3, -march=native, LTO, and loop unrolling,
builds bench_runner's two source files directly with g++, runs it, and
keeps whichever combo has the lowest p50 on the real-world benchmark row
(recommend/global, 4 threads) - but only among combos whose output still
passes bench_runner's own correctness check (optimized path == brute-force
reference). A fast combo that changes the results is rejected, not kept.

Needs a toolchain with LTO support - CLion's bundled MinGW GCC is built
with --disable-lto, so run this under WSL/Linux, not plain Windows:
    wsl -- python3 bench/flag_search.py

Phase 1 (random search): sample `--random` flag combos uniformly at random
(with repeats - this is random search, not a grid sweep) from the full
space of 24 combos, in case a good one random sampling wouldn't obviously
reach gets found "for free".

Phase 2 (evolutionary): seed a population with the best few combos from
phase 1, then for `--generations` rounds, mutate each parent (flip exactly
one flag axis to a different value) to produce children, evaluate them,
and keep the best `--pop-size` of (parents + children) for the next round.
With only 24 possible combos this is really a mutation-based local search
rather than a textbook GA - it's included because it's what was asked for,
not because the search space is large enough to need it over random search
alone.
"""
import argparse
import itertools
import json
import random
import re
import subprocess
import sys
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SOURCES = ["bench/bench_main.cpp", "src/GameStore.cpp"]
FITNESS_ROW = "recommend/global - top-k, 4 threads"

AXES = {
    "opt": ["-O2", "-O3"],
    "march": ["", "-march=native"],
    "lto": ["", "-flto"],
    "unroll": ["", "-funroll-loops", "-fno-unroll-loops"],
}
AXIS_NAMES = list(AXES.keys())


def random_genome():
    return tuple(random.choice(AXES[name]) for name in AXIS_NAMES)


def all_genomes():
    return list(itertools.product(*(AXES[name] for name in AXIS_NAMES)))


def genome_flags(genome):
    return [f for f in genome if f]


def genome_label(genome):
    return " ".join(genome_flags(genome)) or "(baseline)"


def build_and_run(genome, workdir):
    flags = genome_flags(genome)
    binary = workdir / "trial"
    link_flags = ["-flto"] if "-flto" in flags else []

    compile_cmd = ["g++", *flags, "-std=c++20", "-Isrc", "-pthread",
                    *SOURCES, *link_flags, "-o", str(binary)]
    build = subprocess.run(compile_cmd, cwd=REPO_ROOT, capture_output=True, text=True, timeout=120)
    if build.returncode != 0:
        return {"genome": genome, "status": "build_failed", "detail": build.stderr[-500:]}

    run = subprocess.run([str(binary)], cwd=REPO_ROOT, capture_output=True, text=True, timeout=120)
    correct = run.returncode == 0 and "[OK]" in run.stdout
    if not correct:
        return {"genome": genome, "status": "rejected_incorrect", "detail": run.stdout[-500:]}

    match = re.search(rf"{re.escape(FITNESS_ROW)}\s+p50\s+([\d.]+)\s+(us|ns)", run.stdout)
    if not match:
        return {"genome": genome, "status": "no_fitness_row"}
    value, unit = float(match.group(1)), match.group(2)
    p50_us = value if unit == "us" else value / 1000.0
    return {"genome": genome, "status": "ok", "p50_us": p50_us}


def mutate(genome):
    axis_idx = random.randrange(len(AXIS_NAMES))
    axis = AXIS_NAMES[axis_idx]
    choices = [v for v in AXES[axis] if v != genome[axis_idx]]
    if not choices:
        return genome
    child = list(genome)
    child[axis_idx] = random.choice(choices)
    return tuple(child)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--random", type=int, default=15, help="random-search trials (default 15)")
    parser.add_argument("--generations", type=int, default=4, help="evolutionary generations (default 4)")
    parser.add_argument("--pop-size", type=int, default=3, help="survivors kept per generation (default 3)")
    parser.add_argument("--seed", type=int, default=None, help="RNG seed, for a reproducible run")
    args = parser.parse_args()

    if args.seed is not None:
        random.seed(args.seed)

    print(f"Search space: {len(all_genomes())} possible flag combinations\n")

    cache = {}

    def evaluate(genome):
        cached = genome in cache
        if not cached:
            import tempfile
            with tempfile.TemporaryDirectory() as tmp:
                cache[genome] = build_and_run(genome, Path(tmp))
        result = cache[genome]
        tag = {"ok": f"{result.get('p50_us', 0):.1f}us", "build_failed": "BUILD FAILED",
               "rejected_incorrect": "REJECTED (incorrect)", "no_fitness_row": "no fitness row"}[result["status"]]
        if cached:
            tag += " (cached)"
        print(f"  {genome_label(genome):45s} -> {tag}")
        return result

    def is_better(a, b):
        """True if result a beats result b (lower p50, both must be 'ok')."""
        if b is None or b["status"] != "ok":
            return a["status"] == "ok"
        return a["status"] == "ok" and a["p50_us"] < b["p50_us"]

    best = None

    print(f"--- Phase 1: random search ({args.random} trials) ---")
    for _ in range(args.random):
        result = evaluate(random_genome())
        if is_better(result, best):
            best = result

    print(f"\n--- Phase 2: evolutionary search ({args.generations} generations, pop {args.pop_size}) ---")
    valid_so_far = sorted((r for r in cache.values() if r["status"] == "ok"), key=lambda r: r["p50_us"])
    population = [r["genome"] for r in valid_so_far[:args.pop_size]] or [random_genome()]

    for gen in range(1, args.generations + 1):
        print(f" generation {gen}:")
        children = [mutate(g) for g in population]
        for child in children:
            result = evaluate(child)
            if is_better(result, best):
                best = result

        pool = population + children
        scored = [(g, cache[g]) for g in pool if cache[g]["status"] == "ok"]
        scored.sort(key=lambda gr: gr[1]["p50_us"])
        population = [g for g, _ in scored[:args.pop_size]] or population

    print("\n--- Results ---")
    valid = sorted((r for r in cache.values() if r["status"] == "ok"), key=lambda r: r["p50_us"])
    for r in valid[:10]:
        print(f"  {r['p50_us']:8.1f}us  {genome_label(r['genome'])}")

    if best:
        print(f"\nBest: {genome_label(best['genome'])}  ({best['p50_us']:.1f}us p50, correctness verified)")
    else:
        print("\nNo combination both built and passed the correctness check.")

    results_dir = REPO_ROOT / "bench_results"
    results_dir.mkdir(exist_ok=True)
    out_path = results_dir / f"flag_search_{int(time.time())}.json"
    with out_path.open("w") as f:
        json.dump([{**r, "genome": list(r["genome"])} for r in cache.values()], f, indent=2)
    print(f"\nFull trial log: {out_path}")


if __name__ == "__main__":
    sys.exit(main())

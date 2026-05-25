#!/usr/bin/env python3

import json
import sys
from pathlib import Path

VARIANTS = [
    "baseline",
    "unaltered",
    "dummy",
    "no-mutex",
    "no-fd-to-path",
]

SCENARIOS = [
    "seq_big",
    "seq_big_mt",
    "rnd_small",
    "rnd_small_mt",
]

BASELINE_VARIANT = "baseline"

VARIANT_EXCLUDE: dict[str, list[str]] = {
    "no-mutex": ["seq_big_mt", "rnd_small_mt"],
}


def variant_has_scenario(variant: str, scenario: str) -> bool:
    return scenario not in VARIANT_EXCLUDE.get(variant, [])


def parse_job(job: dict, op: str) -> dict | None:
    """
    Parse a single fio read/write section.

    Returns:
        {
            "bw_mib": float,
            "iops": float,
        }
    """
    data = job.get(op, {})

    if data.get("io_bytes", 0) == 0:
        return None

    bw_kib = data.get("bw", 0)
    bw_mib = bw_kib / 1024

    return {
        "bw_mib": bw_mib,
        "iops": data.get("iops", 0),
    }


def load_results(results_dir: Path) -> dict:
    """
    Load all fio JSON results.

    Supports:
      - separate read/write jobs
      - mixed jobs containing both read + write
    """
    results = {}

    for variant in VARIANTS:
        variant_dir = results_dir / variant

        if not variant_dir.is_dir():
            print(
                f"[warn] variant directory not found: {variant_dir}",
                file=sys.stderr,
            )
            continue

        for scenario in SCENARIOS:
            if not variant_has_scenario(variant, scenario):
                continue

            path = variant_dir / f"{scenario}.json"

            if not path.exists():
                print(
                    f"[warn] missing expected result: {path}",
                    file=sys.stderr,
                )
                continue

            try:
                with open(path) as f:
                    data = json.load(f)
            except json.JSONDecodeError:
                print(
                    f"[warn] could not parse {path}",
                    file=sys.stderr,
                )
                continue

            jobs = data.get("jobs", [])

            if not jobs:
                print(
                    f"[warn] no jobs in {path}",
                    file=sys.stderr,
                )
                continue

            #
            # IMPORTANT:
            # Iterate over ALL jobs.
            #
            # This supports:
            #
            #   seq_big:
            #       job0 = read-test
            #       job1 = write-test
            #
            #   seq_big_mt:
            #       single job containing both read/write
            #
            for job in jobs:
                for op in ("read", "write"):
                    parsed = parse_job(job, op)

                    if not parsed:
                        continue

                    results[(variant, scenario, op)] = parsed

    return results


def format_delta(current: float, baseline: float) -> str:
    if baseline == 0:
        return "—"

    delta = ((current - baseline) / baseline) * 100
    sign = "+" if delta >= 0 else ""

    return f"{sign}{delta:.1f}%"


def print_combined_table(results: dict):
    col_variant = 14
    col_op = 6
    col_iops = 12
    col_bw = 14
    col_delta = 10

    header = (
        f"{'Variant':<{col_variant}} "
        f"{'Op':<{col_op}} "
        f"{'IOPS':>{col_iops}} "
        f"{'BW (MiB/s)':>{col_bw}} "
        f"{'IOPS Δ':>{col_delta}}"
    )

    sep = "-" * len(header)

    for scenario in SCENARIOS:

        # skip empty scenarios
        if not any(
            (variant, scenario, op) in results
            for variant in VARIANTS
            for op in ("read", "write")
        ):
            continue

        print()
        print(f"=== {scenario} ===")
        print(sep)
        print(header)
        print(sep)

        for variant in VARIANTS:

            if not variant_has_scenario(variant, scenario):
                continue

            printed_any = False

            for op in ("read", "write"):

                key = (variant, scenario, op)

                if key not in results:
                    continue

                r = results[key]

                base_key = (BASELINE_VARIANT, scenario, op)

                if (
                    variant == BASELINE_VARIANT
                    or base_key not in results
                ):
                    delta_str = "—"
                else:
                    baseline = results[base_key]
                    delta_str = format_delta(
                        r["iops"],
                        baseline["iops"],
                    )

                print(
                    f"{variant:<{col_variant}} "
                    f"{op:<{col_op}} "
                    f"{r['iops']:>{col_iops},.0f} "
                    f"{r['bw_mib']:>{col_bw},.1f} "
                    f"{delta_str:>{col_delta}}"
                )

                printed_any = True

            if printed_any:
                print()

        print(sep)


def main():
    results_dir = (
        Path(sys.argv[1])
        if len(sys.argv) > 1
        else Path("results")
    )

    if not results_dir.exists():
        print(
            f"ERROR: results directory not found: {results_dir}",
            file=sys.stderr,
        )
        sys.exit(1)

    results = load_results(results_dir)

    if not results:
        print(
            "ERROR: no results found. "
            "Have you run ./run_all.sh yet?",
            file=sys.stderr,
        )
        sys.exit(1)

    print(f"\nfio benchmark results — {results_dir.resolve()}\n")
    print(f"  baseline for Δ : {BASELINE_VARIANT}")
    print(f"  variants       : {', '.join(VARIANTS)}")
    print(f"  scenarios      : {', '.join(SCENARIOS)}")

    print_combined_table(results)


if __name__ == "__main__":
    main()

#!/usr/bin/env bash
# run_benchmark.sh — run all fio scenarios for one library variant
# Usage: ./run_benchmark.sh <label> [path/to/libinjector.so]
#
# Examples:
#   ./run_benchmark.sh baseline
#   ./run_benchmark.sh dummy      ../build/dummy/libinjector.so
#   ./run_benchmark.sh no-mutex   ../build/no-mutex/libinjector.so
#   ./run_benchmark.sh mutex      ../build/mutex/libinjector.so
#
# Reproducibility note:
#   - Page cache is dropped before every fio run (requires passwordless sudo,
#     see README for visudo snippet).
#   - fio testfile is created fresh per scenario to avoid cross-contamination.
#   - Results are written to results/<label>/<scenario>.json for later parsing.

set -euo pipefail

LABEL="${1:?Usage: $0 <label> [libpath]}"
LIBPATH="${2:-}"                        # empty = baseline, no LD_PRELOAD
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
RESULTS_DIR="${SCRIPT_DIR}/results/${LABEL}"
FIO_TESTFILE="${SCRIPT_DIR}/fio_testfile"

# Determine which scenarios to run.
# Skip rnd_small_mt if the library is the no-mutex variant.
LIBNAME="$(basename "${LIBPATH:-}")"
if [[ "${LIBNAME}" == "libinjector_no_mutex.so" ]]; then
    FIO_SCENARIOS=(
        "seq_big"
        "rnd_small"
    )
    echo "  [info] no-mutex variant detected — skipping multithreaded scenarios"
else
    FIO_SCENARIOS=(
        "seq_big"
        "seq_big_mt"
        "rnd_small"
        "rnd_small_mt"
    )
fi

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

drop_caches() {
    echo "  [cache] dropping page cache..."
    sync
    echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null
}

run_scenario() {
    local scenario="$1"
    local fio_file="${SCRIPT_DIR}/${scenario}.fio"
    local out_file="${RESULTS_DIR}/${scenario}.json"

    echo "  [fio] scenario: ${scenario}"

    # Remove leftover testfile so each run starts cold
    rm -f "${FIO_TESTFILE}"

    drop_caches

    if [[ -n "${LIBPATH}" ]]; then
        LD_PRELOAD="${LIBPATH}" fio \
            --output-format=json \
            --output="${out_file}" \
            "${fio_file}"
    else
        fio \
            --output-format=json \
            --output="${out_file}" \
            "${fio_file}"
    fi

    echo "  [fio] results written to ${out_file}"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

echo "========================================"
echo " Benchmark variant : ${LABEL}"
echo " Library           : ${LIBPATH:-<none, baseline>}"
echo " Results dir       : ${RESULTS_DIR}"
echo "========================================"

mkdir -p "${RESULTS_DIR}"

# Validate library exists if specified
if [[ -n "${LIBPATH}" && ! -f "${LIBPATH}" ]]; then
    echo "ERROR: library not found: ${LIBPATH}" >&2
    exit 1
fi

for scenario in "${FIO_SCENARIOS[@]}"; do
    run_scenario "${scenario}"
done

# Clean up testfile after all scenarios
rm -f "${FIO_TESTFILE}"

echo ""
echo "Done. All results in: ${RESULTS_DIR}/"

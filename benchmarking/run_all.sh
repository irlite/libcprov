#!/usr/bin/env bash
# run_all.sh — run all 4 benchmark variants back to back
#
# Variants:
#   1. baseline   — no LD_PRELOAD
#   2. dummy      — library preloaded but no-op (measures LD_PRELOAD overhead alone)
#   3. no-mutex   — full injector, mutex disabled
#   4. mutex      — full injector, mutex enabled
#
# Adjust the LIB_* paths below to match your build layout.
#
# Reproducibility:
#   All runs use the same fio scenarios, same cache-drop procedure, and write
#   JSON output to results/<variant>/ for deterministic post-processing.
#   See visudo snippet in README to allow passwordless cache drops.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

LIB_DUMMY="${SCRIPT_DIR}/../build/injector/libinjector_dummy.so"
LIB_NO_MUTEX="${SCRIPT_DIR}/../build/injector/libinjector_no_mutex.so"
LIB_NO_PATH="${SCRIPT_DIR}/../build/injector/libinjector_no_path.so"
LIB_NORMAL="${SCRIPT_DIR}/../build/injector/libinjector.so"

echo "########################################"
echo "#  Full benchmark suite"
echo "#  $(date)"
echo "########################################"
echo ""

bash "${SCRIPT_DIR}/run_benchmark.sh" baseline
echo ""

bash "${SCRIPT_DIR}/run_benchmark.sh" dummy "${LIB_DUMMY}"
echo ""

bash "${SCRIPT_DIR}/run_benchmark.sh" no-mutex "${LIB_NO_MUTEX}"
echo ""

bash "${SCRIPT_DIR}/run_benchmark.sh" no-fd-to-path "${LIB_NO_PATH}"
echo ""

bash "${SCRIPT_DIR}/run_benchmark.sh" unaltered "${LIB_NORMAL}"
echo ""

echo "########################################"
echo "#  All variants complete."
echo "#  Results in: ${SCRIPT_DIR}/results/"
echo "########################################"

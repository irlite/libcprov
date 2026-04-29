#!/usr/bin/env bash
set -euo pipefail

LIB="../build/injector/libinjector.so"
JOB="${1:-rnd_small.fio}"

if [[ ! -f "$LIB" ]]; then
    echo "ERROR: library not found: $LIB" >&2
    exit 1
fi

if [[ ! -f "$JOB" ]]; then
    echo "ERROR: fio job not found: $JOB" >&2
    exit 1
fi

echo "[info] running perf with LD_PRELOAD=$LIB"
echo "[info] fio job: $JOB"

perf record -e task-clock -g -- \
env LD_PRELOAD="$LIB" \
fio "$JOB"

echo "[done] perf.data generated"

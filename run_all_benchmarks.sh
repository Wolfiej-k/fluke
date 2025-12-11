#!/bin/bash

SCRIPT="./run_benchmarks.py"
CONCURRENCIES=(1 2 4 8 16 32 64 128 256)
TRIALS=5

for c in "${CONCURRENCIES[@]}"; do
    prefix="result${c}"
    python3 "$SCRIPT" \
        --trials="$TRIALS" \
        --concurrency="$c" \
        --out-prefix="$prefix"
    echo
done

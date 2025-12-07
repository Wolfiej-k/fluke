#!/bin/bash

LOGFILE="$1"
if [ ! -f "$LOGFILE" ]; then
    echo "Error: Log file '$LOGFILE' not found."
    exit 1
fi

SAFE=$(grep -m1 'Number of total safe checks' "$LOGFILE" | awk '{print $1}')
WARNING=$(grep -m1 'Number of total warning checks' "$LOGFILE" | awk '{print $1}')
ERROR=$(grep -m1 'Number of total error checks' "$LOGFILE" | awk '{print $1}')

SAFE=${SAFE:-0}
WARNING=${WARNING:-0}
ERROR=${ERROR:-0}

TOTAL=$(( WARNING + ERROR ))

echo "FLUKE: verified $SAFE / $TOTAL for $LOGFILE"

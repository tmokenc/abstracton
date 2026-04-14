#!/bin/bash

# adapted from https://hal.science/hal-02391841

set -uo pipefail

TIMEOUT=1200
COMMAND="$1"
DIRECTORIES="$2"
PATTERN="$3"

IFS=' ' read -r -a COMMAND_ARR <<< "$COMMAND"

date
echo "Software : $COMMAND"
echo "Timeout : $TIMEOUT"

for dir in $DIRECTORIES
do
    echo "$dir"

    while IFS= read -r file
    do
        echo "$file"
        if time systemd-run --user --scope \
            -p MemoryMax=32G \
            -p MemorySwapMax=7G \
            timeout "$TIMEOUT" "${COMMAND_ARR[@]}" "$file"
        then
            status=0
        else
            status=$?
            echo "Exit status: $status"
        fi
        echo "********************"
    done < <(find -L "$dir" -type f -name "$PATTERN")

    date
done

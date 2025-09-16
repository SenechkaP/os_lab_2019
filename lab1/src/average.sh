#!/bin/bash

if [ $# -eq 0 ]; then
    echo "no args"
    exit 1
fi

sum=0
count=$#

for i in "$@"; do
    sum=$((sum + i))
done

average=$(echo "scale=2; $sum / $count" | bc)

echo "average = $average"


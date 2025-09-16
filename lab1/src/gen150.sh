#!/bin/bash

for i in {1..150}; do
    num=$(od -An -N1 -tu1 /dev/urandom)
    num=$((num % 101))
    echo $num >> numbers.txt
done

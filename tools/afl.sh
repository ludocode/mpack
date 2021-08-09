#!/bin/bash

# Runs mpack-fuzz under american fuzzy lop.

cd "$(dirname "$0")"/..

export AFL_HARDEN=1
make -f test/fuzz/Makefile || exit 1

echo
echo "This will run the first fuzzer as fuzzer01. To run on additional"
echo "cores, run:"
echo
echo "    afl-fuzz -i test/messagepack -o .build/fuzz/sync -S fuzzer## -- .build/fuzz/mpack-fuzz"
echo
echo "To watch the overall progress, run:"
echo
echo "    watch afl-whatsup .build/fuzz/sync"
echo
echo "Press enter to start..."
read

afl-fuzz -i test/messagepack -o .build/fuzz/sync -M fuzzer01 -- .build/fuzz/mpack-fuzz

#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
make clean >/dev/null 2>&1 || true
make
./oslab --test
./oslab --demo > tests/demo_output.txt
grep -q "Processor Scheduling Demo" tests/demo_output.txt
grep -q "Memory Management Demo" tests/demo_output.txt
grep -q "Process Synchronization Demo" tests/demo_output.txt
grep -q "File System Demo" tests/demo_output.txt
echo "All user-space tests passed."

#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

if [[ "$(id -u)" -ne 0 ]]; then
    echo "This kernel smoke test must run as root:"
    echo "  sudo bash tests/run_kernel_smoke.sh"
    exit 1
fi

make kernel
make proc_client

if [[ ! -f kernel/oslab_proc.ko ]]; then
    echo "kernel/oslab_proc.ko was not generated."
    exit 1
fi

cleanup() {
    rm -f /tmp/oslab_proc_client.out
    if lsmod | awk '{print $1}' | grep -qx "oslab_proc"; then
        rmmod oslab_proc || true
    fi
}
trap cleanup EXIT

if lsmod | awk '{print $1}' | grep -qx "oslab_proc"; then
    rmmod oslab_proc
fi

insmod kernel/oslab_proc.ko
test -r /proc/oslab_monitor

grep -q "OSLab Linux procfs monitor" /proc/oslab_monitor
./proc_client > /tmp/oslab_proc_client.out
grep -q "process_state_count" /tmp/oslab_proc_client.out

echo "note=kernel_smoke_test" > /proc/oslab_monitor
grep -q "note: kernel_smoke_test" /proc/oslab_monitor

echo "reset" > /proc/oslab_monitor
grep -q "note: reset by user" /proc/oslab_monitor

echo "Kernel module smoke test passed."

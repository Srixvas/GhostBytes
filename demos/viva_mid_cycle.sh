#!/bin/bash
TEST_UNIT="gb-viva-$$"
TEST_FILE="/dev/shm/viva_$$.bin"
TRIGGER="/tmp/gb_trigger_$$"
rm -f "$TRIGGER"

cleanup() { 
    systemctl --user stop "${TEST_UNIT}.scope" 2>/dev/null || true
    rm -f "$TEST_FILE" "$TRIGGER"
}
trap cleanup EXIT INT TERM

echo "--> 1. Starting background container in idle mode..."
systemd-run --user --scope --unit="$TEST_UNIT" bash -c "while [ ! -f $TRIGGER ]; do sleep 0.2; done; dd if=/dev/zero of=$TEST_FILE bs=1M count=500 status=none; sync; sleep 300" >/dev/null 2>&1 &
sleep 2

CGROUP_PATH="/sys/fs/cgroup$(systemctl --user show -p ControlGroup --value "${TEST_UNIT}.scope")"

echo "--> 2. Locking WSL-optimized RAM disk thresholds..."
PCT=$(awk '/MemAvailable/ {avail=$2} /MemTotal/ {total=$2} END {printf "%.2f", avail/total*100}' /proc/meminfo)

# The payload is now guaranteed to hit Linux RAM.
export GB_ELEVATED=$(awk "BEGIN {print $PCT - 1.5}")
export GB_CRITICAL=$(awk "BEGIN {print $PCT - 3.5}")

echo "--> 3. Launching timer..."
(
    sleep 6
    echo -e "\n[!] User launched a heavy application. Dropping 500 MiB payload directly into RAM..."
    touch "$TRIGGER"
) &

sleep 1
./build/ghostbyte --test-cgroup "$CGROUP_PATH"

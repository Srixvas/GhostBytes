#!/bin/bash
set -e

TEST_UNIT="gb-settle-demo-$$"
TEST_FILE="settle_data_$$.bin"

cleanup() {
    echo -e "\n--> Cleaning up..."
    systemctl --user stop "${TEST_UNIT}.scope" 2>/dev/null || true
    rm -f "$TEST_FILE"
}
trap cleanup EXIT INT TERM

echo "--> 1. Generating 150 MiB of page cache to simulate a temporary workload..."
systemd-run --user --scope --unit="$TEST_UNIT" bash -c "dd if=/dev/urandom of=$TEST_FILE bs=1M count=150 status=none && sleep 300" >/dev/null 2>&1 &
sleep 4

CGROUP_REL=$(systemctl --user show -p ControlGroup --value "${TEST_UNIT}.scope")
CGROUP_PATH="/sys/fs/cgroup${CGROUP_REL}"

echo "--> 2. Calculating perfect thresholds to demonstrate settling..."
TOTAL=$(awk '/MemTotal/ {print $2}' /proc/meminfo)
AVAIL=$(awk '/MemAvailable/ {print $2}' /proc/meminfo)
PCT=$(awk "BEGIN {printf \"%.2f\", $AVAIL / $TOTAL * 100}")

# Set Elevated 1.0% ABOVE current available memory to trigger reclamation.
# Reclaiming 150MB will boost available RAM by ~4%, easily pushing it back to Healthy.
export GB_ELEVATED=$(awk "BEGIN {print $PCT + 1.0}")
export GB_CRITICAL=$(awk "BEGIN {print $PCT - 5.0}")

echo "Current Available RAM: $PCT %"
echo "Elevated Threshold set to: $GB_ELEVATED % (GhostBytes will wake up here)"
echo "Critical Threshold set to: $GB_CRITICAL %"
echo "--> Handing over to GhostBytes Main Daemon (Press Ctrl+C to stop after it settles)..."
echo "======================================================================"
sleep 1

./build/ghostbyte --test-cgroup "$CGROUP_PATH"

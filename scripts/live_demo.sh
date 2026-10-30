#!/bin/bash
set -e

TEST_UNIT="gb-live-demo-$$"
TEST_FILE="demo_data_$$.bin"

cleanup() {
    echo -e "\n--> Stopping live demo and cleaning up..."
    systemctl --user stop "${TEST_UNIT}.scope" 2>/dev/null || true
    rm -f "$TEST_FILE"
}
trap cleanup EXIT INT TERM

echo "--> Generating 150 MiB of reclaimable memory inside a safe scope..."
systemd-run --user --scope --unit="$TEST_UNIT" bash -c "dd if=/dev/urandom of=$TEST_FILE bs=1M count=150 status=none && sleep 300" >/dev/null 2>&1 &
sleep 4

CGROUP_REL=$(systemctl --user show -p ControlGroup --value "${TEST_UNIT}.scope")
CGROUP_PATH="/sys/fs/cgroup${CGROUP_REL}"

echo "--> Target CGroup loaded."
echo "--> Handing over to GhostBytes Main Daemon (Press Ctrl+C to stop)..."
echo "======================================================================"
sleep 1

./build/ghostbyte --test-cgroup "$CGROUP_PATH"

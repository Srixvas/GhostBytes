#!/usr/bin/env bash
set -Eeuo pipefail

TEST_UNIT="gb-reclaim-test-$$"
SCOPE="${TEST_UNIT}.scope"
TEST_FILE="$(pwd)/test_data_$$.bin"
RUNNER="$(pwd)/build/ghostbyte_reclaim_runner"
REQUEST_BYTES=$((30 * 1024 * 1024))
MIN_MEMORY_BYTES=$((10 * 1024 * 1024))
SCOPE_STARTED=0

cleanup() {
    status=$?
    trap - EXIT INT TERM
    echo "--> Cleaning up scope and test data..."

    if (( SCOPE_STARTED )); then
        systemctl --user stop "$SCOPE" >/dev/null 2>&1 || true
    fi

    rm -f -- "$TEST_FILE"
    exit "$status"
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

fail() {
    echo "ERROR: $*" >&2
    exit 1
}

[[ -x "$RUNNER" ]] || fail "Runner not found: $RUNNER. Build it first."
command -v systemd-run >/dev/null || fail "systemd-run is unavailable."
command -v systemctl >/dev/null || fail "systemctl is unavailable."

UID_NUM="$(id -u)"
DELEGATED_PREFIX="/user.slice/user-${UID_NUM}.slice/user@${UID_NUM}.service/"

echo "--> Starting systemd user scope: $SCOPE"
systemd-run --user --scope --unit="$TEST_UNIT" \
    bash -c 'dd if=/dev/urandom of="$1" bs=1M count=50 status=none && sleep 300' \
    _ "$TEST_FILE" >/dev/null 2>&1 &
LAUNCH_PID=$!
SCOPE_STARTED=1

CGROUP_REL=""
for _ in $(seq 1 30); do
    CGROUP_REL="$(systemctl --user show -p ControlGroup --value "$SCOPE" 2>/dev/null || true)"
    if [[ -n "$CGROUP_REL" && "$CGROUP_REL" != "/" ]]; then
        break
    fi
    sleep 1
done

[[ -n "$CGROUP_REL" && "$CGROUP_REL" != "/" ]] ||
    fail "Could not resolve the scope's cgroup."

[[ "$CGROUP_REL" == "$DELEGATED_PREFIX"* ]] ||
    fail "Scope is outside the delegated user cgroup subtree: $CGROUP_REL"

CGROUP_PATH="/sys/fs/cgroup${CGROUP_REL}"
[[ -d "$CGROUP_PATH" ]] || fail "Cgroup directory does not exist: $CGROUP_PATH"
[[ -r "$CGROUP_PATH/memory.current" ]] ||
    fail "Cannot read memory.current for target cgroup."

echo "--> Target cgroup: $CGROUP_PATH"
echo "--> Waiting for at least 10 MiB to be charged to the scope..."

MEM_BEFORE=""
for _ in $(seq 1 30); do
    MEM_BEFORE="$(cat "$CGROUP_PATH/memory.current" 2>/dev/null || true)"
    if [[ "$MEM_BEFORE" =~ ^[0-9]+$ ]] && (( MEM_BEFORE >= MIN_MEMORY_BYTES )); then
        break
    fi
    sleep 1
done

[[ "$MEM_BEFORE" =~ ^[0-9]+$ ]] ||
    fail "Could not obtain a valid pre-reclamation memory measurement."
(( MEM_BEFORE >= MIN_MEMORY_BYTES )) ||
    fail "Scope never reached the minimum memory threshold; observed ${MEM_BEFORE} bytes."

echo "--> Scope memory.current before reclaim: $MEM_BEFORE bytes"
echo "--> Requesting reclamation of 30 MiB..."

OUTPUT="$("$RUNNER" "$CGROUP_PATH" "$REQUEST_BYTES" 2>&1)" || {
    printf '%s\n' "$OUTPUT"
    fail "Reclamation runner exited unsuccessfully."
}
printf '%s\n' "$OUTPUT"

field() {
    awk -F ':' -v key="$1" '
        {
            label = $1
            gsub(/^[[:space:]]+|[[:space:]]+$/, "", label)
            if (label == key) {
                value = substr($0, index($0, ":") + 1)
                gsub(/^[[:space:]]+|[[:space:]]+$/, "", value)
                sub(/ bytes$/, "", value) # FIX: Strip the bytes suffix
                print value
                exit
            }
        }
    ' <<< "$OUTPUT"
}

ACTION="$(field "Action Taken")"
MEASUREMENT="$(field "Measurement Valid")"
BEFORE="$(field "Memory Before")"
AFTER="$(field "Memory After")"
OBSERVED="$(field "Observed Reclaimed")"

[[ "$ACTION" == "Yes" ]] || fail "Runner did not confirm that an action was taken."
[[ "$MEASUREMENT" == "Yes" ]] || fail "Runner did not confirm a valid measurement."
[[ "$BEFORE" =~ ^[0-9]+$ ]] || fail "Runner's Memory Before value is missing or invalid."
[[ "$AFTER" =~ ^[0-9]+$ ]] || fail "Runner's Memory After value is missing or invalid."
[[ "$OBSERVED" =~ ^[0-9]+$ ]] || fail "Runner's Observed Reclaimed value is missing or invalid."

(( BEFORE > AFTER )) ||
    fail "Memory did not decrease according to the runner's measurements."

(( OBSERVED == BEFORE - AFTER )) ||
    fail "Observed Reclaimed does not match Memory Before minus Memory After."

echo
echo "PASS: Real reclamation request submitted and measured."
echo "      Memory decrease: $OBSERVED bytes"
echo "      Note: net cgroup memory change is not exclusive proof of causation."

#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/npc-trigger-tests.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM

run_test() {
    suite_name=$1
    shift
    "${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -pedantic \
        -fsanitize=address,undefined -fno-omit-frame-pointer \
        "$@" -o "$test_binary"
    # LeakSanitizer cannot inspect threads in ptrace-based build sandboxes.
    ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$test_binary"
    printf '%s passed\n' "$suite_name"
}

run_test 'Trigger runtime' \
    "$project_dir/tests/trigger_tests.cpp" \
    "$project_dir/src/main/jni/Trigger/TriggerRuntime.cpp"
run_test 'Trigger dot appearance' "$project_dir/tests/trigger_dot_style_test.cpp"
# This binary stubs the loader and driver to exercise the Android initialization
# policy on the host; the actual Android hook implementation is not linked.
run_test 'Native profile' \
    -D__ANDROID__ -D__aarch64__ -Wl,--wrap=open \
    "$project_dir/tests/native_profile_tests.cpp" \
    "$project_dir/src/main/jni/Trigger/NativeProfile.cpp" \
    "$project_dir/src/main/jni/Trigger/TriggerCompatibility.cpp"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$test_binary" --permanent
printf '%s\n' 'Native profile permanent rejection passed'

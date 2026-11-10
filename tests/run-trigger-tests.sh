#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/npc-trigger-tests.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM

"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -pedantic \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    "$project_dir/tests/trigger_tests.cpp" \
    "$project_dir/src/main/jni/Trigger/TriggerRuntime.cpp" \
    -o "$test_binary"
# LeakSanitizer cannot inspect threads in ptrace-based build sandboxes.
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$test_binary"

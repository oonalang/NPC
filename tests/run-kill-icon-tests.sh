#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/npc-kill-icon-tests.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I"$project_dir/src/main/jni" \
    "$project_dir/tests/kill_icon_test.cpp" -o "$test_binary"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$test_binary"

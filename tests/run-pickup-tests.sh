#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/npc-pickup-tests.XXXXXX")
config_dir=$(mktemp -d "${TMPDIR:-/tmp}/npc-pickup-config.XXXXXX")
trap 'rm -f "$test_binary"; rm -rf "$config_dir"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I"$project_dir/src/main/jni/System" \
    "$project_dir/tests/pickup_test.cpp" \
    "$project_dir/src/main/jni/Trigger/NativeProfile.cpp" -lpthread -o "$test_binary"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$test_binary" "$config_dir" "$@"

#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/npc-session-tests.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -pedantic \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    "$project_dir/tests/session_protection_test.cpp" -lpthread -o "$test_binary"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$test_binary"
"${CXX:-c++}" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -D__ANDROID__ -D__aarch64__ "$project_dir/tests/session_protection_native_test.cpp" \
    "$project_dir/src/main/jni/System/Hooks/SessionProtection.cpp" \
    "$project_dir/src/main/jni/Trigger/NativeProfile.cpp" -lpthread -o "$test_binary"
for mode in normal --stock-on --cold --mismatch --throw --build --prefix --partial; do
    ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$test_binary" "$mode"
done

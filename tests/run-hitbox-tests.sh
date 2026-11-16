#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/npc-hitbox-tests.XXXXXX")
config_dir=$(mktemp -d "${TMPDIR:-/tmp}/npc-hitbox-config.XXXXXX")
trap 'rm -f "$test_binary"; rm -rf "$config_dir"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    "$project_dir/tests/hitbox_test.cpp" -lpthread -o "$test_binary"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$test_binary"
"${CXX:-c++}" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -I"$project_dir/src/main/jni/System" "$project_dir/tests/hitbox_config_test.cpp" -o "$test_binary"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$test_binary" "$config_dir"
"${CXX:-c++}" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -D__ANDROID__ -D__aarch64__ "$project_dir/tests/hitbox_profile_test.cpp" \
    "$project_dir/src/main/jni/System/Hooks/Hitbox.cpp" \
    "$project_dir/src/main/jni/Trigger/NativeProfile.cpp" "$project_dir/tests/license_gate_stub.cpp" -lpthread -o "$test_binary"
for mode in '' --build --anchor --partial --sphere-anchor --sphere-partial --obb-anchor; do
    ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$test_binary" $mode
done
"${CXX:-c++}" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    -D__ANDROID__ -D__aarch64__ "$project_dir/tests/hitbox_native_test.cpp" \
    "$project_dir/src/main/jni/Trigger/NativeProfile.cpp" "$project_dir/tests/license_gate_stub.cpp" -lpthread -o "$test_binary"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$test_binary"

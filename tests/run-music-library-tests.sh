#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/npc-music-library-tests.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM

"${CXX:-c++}" -std=c++17 -O1 -g -DCLAY_CPU_PREVIEW \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -ffunction-sections -fdata-sections -I"$project_dir/src/main/jni" \
    "$project_dir/tests/music_library_test.cpp" \
    "$project_dir/src/main/jni/ImGui/imgui.cpp" \
    "$project_dir/src/main/jni/ImGui/imgui_draw.cpp" \
    "$project_dir/src/main/jni/ImGui/imgui_widgets.cpp" \
    "$project_dir/src/main/jni/ImGui/imgui_tables.cpp" \
    "$project_dir/src/main/jni/System/Audio/DriveMusic.cpp" \
    -Wl,--gc-sections -lpthread -o "$test_binary"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$test_binary"

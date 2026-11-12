#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/npc-dropdown-tests.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM

# Exercise the actual ImGui controls and input queue without a GPU dependency.
"${CXX:-c++}" -std=c++17 -O0 -g -DCLAY_CPU_PREVIEW \
    -ffunction-sections -fdata-sections -I"$project_dir/src/main/jni" \
    "$project_dir/tests/dropdown_interaction_test.cpp" \
    "$project_dir/src/main/jni/ImGui/imgui.cpp" \
    "$project_dir/src/main/jni/ImGui/imgui_draw.cpp" \
    "$project_dir/src/main/jni/ImGui/imgui_widgets.cpp" \
    "$project_dir/src/main/jni/ImGui/imgui_tables.cpp" \
    "$project_dir/src/main/jni/ImGui/custom_widgets.cpp" \
    -Wl,--gc-sections -o "$test_binary"
"$test_binary"

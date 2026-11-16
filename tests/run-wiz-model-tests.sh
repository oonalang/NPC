#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/npc-wiz-tests.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -O2 -DCLAY_CPU_PREVIEW -fsanitize=address,undefined -fno-omit-frame-pointer \
 "$project_dir/tests/wiz_model_test.cpp" "$project_dir/src/main/jni/ImGui/WizModel.cpp" "$project_dir/src/main/jni/ImGui/WizMotion.cpp" -lz -lpthread -o "$test_binary"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$test_binary" "$project_dir/src/main/jni/Assets/wiz-model.bin" "$@"

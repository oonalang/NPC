#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
output_dir=${1:-/tmp/npc-wiz-preview}
header_dir=${GLES_INCLUDE_DIR:-/usr/include}
if [ ! -f "$header_dir/EGL/egl.h" ] && [ -d /workspace/glass-validation/include ]; then
    header_dir=/workspace/glass-validation/include
fi
mkdir -p "$output_dir"
test_binary=$(mktemp "${TMPDIR:-/tmp}/npc-wiz-gpu.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -O2 -DWL_EGL_PLATFORM -I"$header_dir" \
 -DWIZ_MODEL_FILE=\""$project_dir/src/main/jni/Assets/wiz-model.bin"\" \
 "$project_dir/tests/wiz_model_gpu_test.cpp" "$project_dir/src/main/jni/ImGui/WizModel.cpp" "$project_dir/src/main/jni/Assets/WizModel.S" \
 -Wl,-l:libEGL.so.1,-l:libGL.so.1 -lz -lpthread -o "$test_binary"
LIBGL_ALWAYS_SOFTWARE=1 "$test_binary" "$output_dir"

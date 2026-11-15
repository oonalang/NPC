#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
output_dir=${1:-/tmp/npc-wiz-preview}
header_dir=${GLES_INCLUDE_DIR:-/usr/include}
if [ ! -f "$header_dir/EGL/egl.h" ] && [ -d /workspace/glass-validation/include ]; then
    header_dir=/workspace/glass-validation/include
fi
mkdir -p "$output_dir"
build_dir=$(mktemp -d "${TMPDIR:-/tmp}/npc-wiz-gpu.XXXXXX")
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM
mkdir "$build_dir/common"
for source in tests/wiz_model_gpu_test.cpp src/main/jni/ImGui/WizModel.cpp src/main/jni/ImGui/WizMotion.cpp \
 src/main/jni/ImGui/imgui.cpp src/main/jni/ImGui/imgui_draw.cpp src/main/jni/ImGui/imgui_widgets.cpp src/main/jni/ImGui/imgui_tables.cpp src/main/jni/Assets/AnimeArtwork.S; do
    name=$(basename "$source")
    "${CXX:-c++}" -std=c++17 -O2 -DWL_EGL_PLATFORM -ffunction-sections -fdata-sections -I"$header_dir" -I"$project_dir/src/main/jni" \
     -DANIME_ARTWORK_FILE=\""$project_dir/src/main/jni/Assets/wiz-preview.png"\" \
     -c "$project_dir/$source" -o "$build_dir/common/$name.o"
done
for mode in normal failure; do
    model_file=$project_dir/src/main/jni/Assets/wiz-model.bin
    if [ "$mode" = failure ]; then
        model_file=$build_dir/unsupported.bin
        printf 'NOT_A_MODEL!' > "$model_file"
    fi
    "${CXX:-c++}" -DWIZ_MODEL_FILE=\""$model_file"\" -c "$project_dir/src/main/jni/Assets/WizModel.S" -o "$build_dir/model.o"
    "${CXX:-c++}" "$build_dir"/common/*.o "$build_dir/model.o" -Wl,--gc-sections,-l:libEGL.so.1,-l:libGL.so.1 -lz -lpthread -o "$build_dir/test"
    if [ "$mode" = failure ]; then
        LIBGL_ALWAYS_SOFTWARE=1 "$build_dir/test" "$output_dir" --bad-asset
    else
        LIBGL_ALWAYS_SOFTWARE=1 "$build_dir/test" "$output_dir"
    fi
done

#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/ethnir-lite-tests.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM
headers=${EGL_INCLUDE_DIR:-/usr/include}
if [ ! -f "$headers/EGL/egl.h" ] && [ -d /workspace/glass-validation/include ]; then
    headers=/workspace/glass-validation/include
fi
if [ "${LITE_GPU_TEST:-0}" = 1 ]; then
    extra_flags="-DLITE_GPU_TEST -DWL_EGL_PLATFORM -DIMGUI_IMPL_OPENGL_ES3 -I$headers"
    extra_sources="$project_dir/src/main/jni/ImGui/imgui_impl_opengl3.cpp $project_dir/src/main/jni/Lite/FrostedSidebar.cpp"
    extra_libraries="-Wl,-l:libEGL.so.1,-l:libGL.so.1"
else
    extra_flags="-fsanitize=address,undefined -fno-omit-frame-pointer"
    extra_sources=""
    extra_libraries=""
fi
"${CXX:-c++}" -std=c++17 -O1 -g -DETHNIR_LITE=1 -ffunction-sections -fdata-sections $extra_flags \
    -I"$project_dir/src/main/jni" -I"$project_dir/src/main/jni/System" \
    "$project_dir/tests/lite_ui_test.cpp" \
    "$project_dir/src/main/jni/Lite/WidgetBindings.cpp" \
    "$project_dir/src/main/jni/Trigger/TriggerRuntime.cpp" \
    "$project_dir/src/main/jni/ImGui/imgui.cpp" "$project_dir/src/main/jni/ImGui/imgui_draw.cpp" \
    "$project_dir/src/main/jni/ImGui/imgui_widgets.cpp" "$project_dir/src/main/jni/ImGui/imgui_tables.cpp" \
    $extra_sources -Wl,--gc-sections -lpthread $extra_libraries -o "$test_binary"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" MESA_SHADER_CACHE_DISABLE=true "$test_binary" "$project_dir" "${1:-${TMPDIR:-/tmp}/ethnir-lite-frames}"

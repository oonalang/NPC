#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
if [ -n "${NDK_BUILD:-}" ]; then
    ndk_command=$NDK_BUILD
elif [ -n "${ANDROID_NDK_HOME:-}" ]; then
    ndk_command=$ANDROID_NDK_HOME/ndk-build
elif [ -n "${ANDROID_NDK_ROOT:-}" ]; then
    ndk_command=$ANDROID_NDK_ROOT/ndk-build
else
    ndk_command=ndk-build
fi
if ! command -v "$ndk_command" >/dev/null 2>&1; then
    printf '%s\n' 'Set NDK_BUILD to the ndk-build executable, or add the Android NDK to PATH.' >&2
    exit 1
fi
"$ndk_command" -C "$project_dir/src/main" -j2 "$@" \
    NDK_PROJECT_PATH="$project_dir/src/main" \
    APP_BUILD_SCRIPT="$project_dir/src/main/jni/Android.mk" \
    NDK_APPLICATION_MK="$project_dir/src/main/jni/Application.mk" \
    NDK_OUT="$project_dir/src/main/obj" \
    NDK_LIBS_OUT="$project_dir/src/main/libs"
sh "$project_dir/tools/verify-lite-library.sh" "$project_dir/src/main/libs/arm64-v8a/libnets.so"

#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/npc-drive-audio-tests.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    "$project_dir/tests/drive_audio_test.cpp" \
    "$project_dir/src/main/jni/System/Audio/DriveMusic.cpp" \
    "$project_dir/src/main/jni/System/Audio/UiSounds.cpp" -o "$test_binary"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$test_binary" "$@"

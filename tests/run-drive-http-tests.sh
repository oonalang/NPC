#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
test_binary=$(mktemp "${TMPDIR:-/tmp}/npc-drive-http-tests.XXXXXX")
trap 'rm -f "$test_binary"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -O1 -DDRIVE_HTTP_TEST \
    "-DDRIVE_TEST_CA_FILE=\"$project_dir/src/main/jni/Assets/drive-ca.pem\"" \
    "$project_dir/tests/drive_music_http_test.cpp" \
    "$project_dir/src/main/jni/System/Audio/DriveMusic.cpp" -lcurl -o "$test_binary"
"$test_binary" "$1"

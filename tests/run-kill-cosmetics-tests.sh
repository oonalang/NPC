#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
output=$(mktemp "${TMPDIR:-/tmp}/kill-cosmetics-tests.XXXXXX")
config_dir=$(mktemp -d "${TMPDIR:-/tmp}/kill-cosmetics-config.XXXXXX")
trap 'rm -f "$output"; rm -rf "$config_dir"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
 "$project_dir/tests/kill_cosmetics_test.cpp" -lpthread -o "$output"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$output"
"${CXX:-c++}" -std=c++17 -O1 -g -DETHNIR_LITE=1 -fsanitize=address,undefined -fno-omit-frame-pointer \
 -I"$project_dir/src/main/jni/System" "$project_dir/tests/kill_cosmetics_config_test.cpp" -o "$output"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$output" "$config_dir"

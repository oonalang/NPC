#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
binary=$(mktemp "${TMPDIR:-/tmp}/shai-lite-check.XXXXXX")
trap 'rm -f "$binary"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
    "$root/tests/lite_profile_test.cpp" -o "$binary"
"$binary"
printf '%s\n' 'SHAI Lite feature profile passed'

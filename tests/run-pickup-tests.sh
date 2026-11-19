#!/bin/sh
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
binary=$(mktemp "${TMPDIR:-/tmp}/npc-pickup-test.XXXXXX")
trap 'rm -f "$binary"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror -pedantic -pthread \
    "$root/tests/pickup_tweaks_test.cpp" -o "$binary"
"$binary"
printf '%s\n' 'BR pickup settings tests passed'

#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
binary=$(mktemp "${TMPDIR:-/tmp}/npc-loot-labels.XXXXXX")
trap 'rm -f "$binary"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Werror -pedantic \
    "$project_dir/tests/loot_label_clusters_test.cpp" -o "$binary"
"$binary"
printf '%s\n' 'Loot label clusters passed'

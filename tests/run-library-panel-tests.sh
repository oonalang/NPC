#!/bin/sh
set -eu

project_dir=$(cd "$(dirname "$0")/.." && pwd)
PHP=${PHP:-php}

if ! command -v "$PHP" >/dev/null 2>&1; then
    echo "SKIP: no PHP CLI available (set PHP=/path/to/php)"
    exit 0
fi

if ! command -v curl >/dev/null 2>&1; then
    echo "SKIP: curl is required for the panel upload test"
    exit 0
fi

work=$(mktemp -d)
cleanup() {
    rm -rf "$work"
}
trap cleanup EXIT HUP INT TERM

cp -R "$project_dir/server/license/." "$work/"
rm -f "$work/config.php"

"$PHP" "$project_dir/tests/library_panel_test.php" "$work"

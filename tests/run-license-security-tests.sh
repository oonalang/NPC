#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
output=$(mktemp -d "${TMPDIR:-/tmp}/npc-license-tests.XXXXXX")
trap 'rm -rf "$output"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
    "$project_dir/tests/license_response_test.cpp" -lcrypto -lpthread -o "$output/response"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$output/response"
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
    "$project_dir/tests/license_session_test.cpp" -lcrypto -lpthread -o "$output/session"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$output/session"
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
    "$project_dir/tests/license_gravity_test.cpp" "$project_dir/tests/license_gate_stub.cpp" -o "$output/gravity"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$output/gravity"
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
    "$project_dir/tests/license_native_utilities_test.cpp" "$project_dir/tests/license_gate_stub.cpp" -o "$output/native"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$output/native"
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
    "$project_dir/tests/license_runtime_test.cpp" "$project_dir/src/main/jni/Security/LicenseRuntime.cpp" -lcrypto -lpthread -o "$output/runtime"
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" "$output/runtime"
"${CXX:-c++}" -std=c++17 -O1 -g -Wall -Wextra -Werror \
    "$project_dir/tests/lease_verify_cli.cpp" -lcrypto -o "$output/verify"
python3 "$project_dir/tests/license_server_test.py" --php "${PHP:-php}" --verifier "$output/verify"
python3 "$project_dir/tests/license_panel_test.py" --php "${PHP:-php}" --verifier "$output/verify"

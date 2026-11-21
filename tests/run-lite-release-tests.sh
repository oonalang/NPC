#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
output=$(mktemp -d "${TMPDIR:-/tmp}/ethnir-lite-release.XXXXXX")
trap 'rm -rf "$output"' EXIT HUP INT TERM
openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out "$output/key.pem" 2>/dev/null
openssl pkey -in "$output/key.pem" -pubout -outform DER -out "$output/public.der"
openssl pkcs8 -in "$output/key.pem" -topk8 -nocrypt -outform DER -out "$output/private.der"
python3 - "$output" <<'PY'
import base64
import hashlib
from pathlib import Path
import sys
out=Path(sys.argv[1])
der=(out/'public.der').read_bytes()
(out/'LicenseRoot.java').write_text('package com.android.support; final class LicenseRoot { static final String KEY_ID="'+hashlib.sha256(der).hexdigest()+'"; static final String PUBLIC_KEY="'+base64.b64encode(der).decode()+'"; }')
(out/'library.so').write_bytes(bytes(range(256))*4)
PY
python3 "$project_dir/tools/sign-native-release.py" "$output/library.so" --product ETHNIR_LITE \
    --private-key "$output/key.pem" --out "$output/library.manifest"
"${JAVAC:-javac}" -source 8 -target 8 -d "$output/classes" \
    "$project_dir/tests/java/android/util/Base64.java" "$output/LicenseRoot.java" \
    "$project_dir/src/main/java/com/android/support/VerifiedLibrary.java" \
    "$project_dir/tests/java/com/android/support/VerifiedLibraryTest.java"
"${JAVA:-java}" -cp "$output/classes" com.android.support.VerifiedLibraryTest \
    "$output/library.manifest" "$output/library.so" "$output/private.der"

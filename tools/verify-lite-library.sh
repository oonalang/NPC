#!/bin/sh
set -eu
if [ "$#" -ne 1 ]; then
    printf '%s\n' 'Usage: sh tools/verify-lite-library.sh /path/to/libnets.so' >&2
    exit 1
fi
library=$1
if [ ! -r "$library" ] || [ ! -f "$library" ]; then
    printf 'Library does not exist or is unreadable: %s\n' "$library" >&2
    exit 1
fi
signature=$(od -An -v -tx1 -N20 "$library" | tr -d '[:space:]')
case "$signature" in
    7f454c460201*0300b700) ;;
    *) printf '%s\n' 'Expected a little-endian ARM64 ELF shared library.' >&2; exit 1 ;;
esac
if ! LC_ALL=C grep -Fq 'SHAINIME##menu' "$library"; then
    printf '%s\n' 'SHAINIME UI is missing. This is a stale or different-edition library.' >&2
    exit 1
fi
if LC_ALL=C grep -Fq -e 'NPC EXECUTIVE AUTH' -e 'ESP PREVIEW' "$library"; then
    printf '%s\n' 'Full NPC UI is present. Rebuild Lite before using this library.' >&2
    exit 1
fi
size=$(wc -c < "$library" | tr -d '[:space:]')
if [ "$size" -gt 5242880 ]; then
    printf 'Lite library exceeds the 5 MiB release budget: %s bytes. Check stripping and build inputs.\n' "$size" >&2
    exit 1
fi
printf 'Verified SHAINIME ARM64 library: %s\nBytes: %s\n' "$library" "$size"
if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$library"
fi

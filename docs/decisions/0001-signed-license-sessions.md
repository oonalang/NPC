# ADR-001: Signed license sessions and native release verification

## Status

Implemented and authorized for publication to `main`; hosted rollout pending.

## Context

The user identified panel swapping and patching a login boolean as common bypasses. The original client verified RSA signatures, but accepted long-lived replayable v1 responses and used `bValid`/`isLogin` for presentation. Several features were installed before login. The launcher downloaded a native library from another host and loaded either that download or an unchecked cached file. Release builds also inherited debug signing.

## Decision

Require protocol v2 on both client and panel. Every request has a 256-bit OpenSSL random nonce. The RSA-SHA256 signature binds audience, build, key, serial, nonce, issuance, session expiry, license expiry, capabilities and public-key identifier. A grant lasts at most 180 seconds; renewal occurs after 60 seconds, with 10-second retries for transient failures. Rejected or unverifiable responses revoke access. Transient errors retain only the unexpired prior grant.

An immutable verified session, bounded by `CLOCK_BOOTTIME`, authorizes native feature callbacks separately from the menu. Loading-animation completion grants no authority. Native gates delegate to originals after revocation; FPS fields and owned gravity/air-control fields have restoration paths. Existing preference values remain intact.

Require a signed SHA-256 release manifest before downloading/loading the native library. The launcher and native client share the same public trust root; the private signing key stays outside the repository. Distribution moves to the user's existing `ethnir.x10.mx` host. Native builds restrict exports to JNI entry points and use RELRO, immediate binding, a non-executable stack, stack protection and FORTIFY. Release APKs no longer automatically inherit the debug certificate.

## Alternatives considered

Renaming login flags, hiding the endpoint with XOR, or adding debugger/process-name blacklists does not authenticate a server response. Heavy runtime packing and virtualized code would add unsupported build dependencies and runtime cost without removing client control. The implemented trust checks use the existing OpenSSL/curl/Android architecture.

## Consequences

Server and client must be upgraded together; there is no v1 fallback. The PHP update preserves the existing admin configuration, keys, device lists and seat rules, while serializing store mutations and saving the store atomically. Free/Premium status is a signed capability: masks `3` and `7` respectively. The user's full three-ad/ten-hour panel is retained, including its layout, with per-claim proof deduplication and a common lock for final issuance; provider network checks occur outside that lock. Deployments must protect and back up the private key and existing data. The complete private ZIP preserves supplied config except its matching public modulus and contains no live database. The launcher APK must be rebuilt; existing installed APKs retain their old download behavior.

A patched client can still remove local checks. Publicly shared source can be rebuilt without them. The software-derived serial can be spoofed by a controlled client; it is not hardware attestation. Static release manifests prevent unauthorized replacement, but do not provide a server-enforced monotonic release counter. Previously applied cosmetic/game state cannot all be reconstructed on revocation. These limits are explicit; this change is not an uncrackability claim.

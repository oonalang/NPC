# License and release protection

The hardened client is on `feature/license-session-hardening`, based on main `fdd0c16`. It requires matching PHP files and a rebuilt launcher. The existing hosted installation has not been changed.

## Protocol

`POST https://ethnir.x10.mx/connect.php` accepts `protocol=2`, `game=CODMGR`, `build=1`, the normalized key, the existing device serial and a fresh 64-character lowercase hex nonce. Nonces contain 32 bytes from OpenSSL's CSPRNG. Requests use HTTPS certificate and hostname verification, system trust roots, no redirects and bounded connect/read/response sizes.

A successful JSON response contains `data.payload` and `data.sig`. The exact signed payload is:

```text
v2|CODMGR|1|key|serial|nonce|issued|lease_until|license_until|capabilities|key_id
```

RSA-SHA256 authenticates every field. Decimal values must be canonical. Unknown versions, audiences, builds, keys, devices, nonces, capabilities, key identifiers, invalid lifetimes and signatures fail closed. `key_id` is the SHA-256 fingerprint of the public key's DER SubjectPublicKeyInfo. Capability bit 1 grants menu access; bit 2 grants native features. The panel currently issues both.

Grants last at most 180 seconds, bounded by the license expiry. Revalidation runs on a worker after 60 seconds; transient failures retry after 10 seconds. A rejected or unverifiable renewal revokes the grant. An interrupted connection retains only the previous unexpired grant. Normal revocation is learned at renewal; network failures can delay it until that grant expires. No offline or v1 fallback exists.

Acceptance also records a deadline against `CLOCK_BOOTTIME`, including device suspend, bounded from the start of the request. Changing the phone's wall clock cannot extend an accepted grant. Pending generations and nonce matching reject replayed responses and late workers after a newer login. Neither finishing the loading animation nor assigning a local presentation flag creates a session. A verified renewal can resume an already opened menu after a connection interruption; an explicit new login resets the startup presentation. The device clock must be within 30 seconds of the signing server at verification.

## Native enforcement

`Security/Lease.h` constructs immutable grants only after verifying the signature and claims. `Security/LicenseRuntime.cpp` owns the active session and renewal worker. Native entry points consult authorization independently of menu presentation. Preferences remain separate from authorization.

Legacy memory hooks and the bullet/cosmetic runtime hooks use typed wrappers that forward arguments to the original function when authorization is absent. Hitbox, trigger input, slide fatigue, frame-rate overrides and session protection also enforce authorization within their native paths. Feature installation is deferred until signed login; capture-only skin/config collectors remain available during startup so existing catalog behavior is preserved.

Expired/revoked grants stop new overrides. FPS device fields, owned gravity/air-control values, active patches and the observed CODM session-protection policy have restoration paths. In-flight calls may finish. Already spawned/applied cosmetics and every legacy persistent game-state change cannot all be reconstructed; this does not promise complete rollback of prior actions.

## Native download verification

The rebuilt launcher reads `https://ethnir.x10.mx/native/libZANEV1.manifest`. Before loading a cached file or installing a download from `/native/libZANEV1.so`, it verifies the publisher's signature, expected product/build/filename/size and SHA-256 hash. Redirects and other hosts are rejected. A partial download never replaces the active library; the candidate file is read-only and verified before an atomic rename. Failed verification does not load an unchecked cached file.

The release manifest consists of the following payload, newline, its canonical Base64 RSA-SHA256 signature, and a final newline:

```text
v1|SHAI_ETHNIR|1|libZANEV1.so|byte_length|sha256|key_id
```

The manifest is a release-integrity check, not a session grant or monotonic anti-rollback counter. Native session authorization remains mandatory. Existing installed launchers keep their old behavior until their APK is replaced.

## Deploy this build

1. Back up the live panel files, current signing key and license JSON. Keep existing `config.php`, admin credentials, data and device bindings.
2. The prepared `shai-public-update.zip` contains three PHP files and the native library/manifest. Upload it into the panel's existing web root, replacing `connect.php` and `lib.php`, and adding `SessionProtocol.php` and `native/`.
3. The separate `shai-private-key.zip` contains the matching private signing key and deny rules. Keep this archive private. Install the key at the path configured by `config.php`'s `private_key`; the supplied panel's default is `private/licence_private.pem`. Prefer a path outside the web root. Restrict the key to the PHP account and verify the browser cannot retrieve it. The current client/public manifest must use this exact key pair.
4. Check the panel still uses build `1`. Do not replace `config.php` or delete the existing license store. The new `lib.php` supports the existing admin API and serializes POST mutations, including seat allocation and admin changes. It refuses malformed stores and preserves the original store when a temporary write is incomplete.
5. Rebuild the launcher APK from this branch using your release signing certificate, then distribute it with the server update. Release builds no longer inherit the debug signing certificate. No production keystore or signed APK was supplied or generated here.
6. With a test key on an Android device, verify initial login, renewal past one minute, expiry/revocation, seat limits and denial of tampered releases. Both old clients and old panels need upgrading; protocol v1 receives HTTP 426. Uploading source alone does not update installed libraries or APKs.

No private key is stored in git. Public trust roots are in `Security/LicenseRoot.h` and `LicenseRoot.java`. The current generated key is outside the checkout at `/workspace/license-hardening/keys/licence_private.pem`, and deployment archives are under `/workspace/license-hardening/artifacts/`. Back up the private archive securely; losing the private key requires another coordinated rotation.

For later rotations, generate a new private key outside the checkout with `python3 tools/license-keys.py --private-out /secure/new-signing-key.pem`, rebuild both native library and launcher, and redeploy the matching panel key. Do not run that command again for the already prepared binary.

For later native releases using the same root:

```sh
python3 tools/package-license-release.py \
  --library /path/to/libnets.so \
  --private-key /secure/signing-key.pem \
  --output-dir /path/outside/the/checkout
```

The packager checks the source/binary/key match, verifies ELF hardening and creates separate public and private archives. Never publish the private archive as a downloadable release asset.

## Validation

Run with a PHP CLI that has OpenSSL and a JDK compiler:

```sh
sh tests/run-license-security-tests.sh
python3 tests/run-release-verifier-tests.py
sh tests/run-hitbox-tests.sh
sh tests/run-session-protection-tests.sh
sh tests/run-trigger-tests.sh
sh tests/run-frame-rate-tests.sh
sh tests/run-slide-fatigue-tests.sh
python3 tools/verify-native-hardening.py /path/to/libnets.so
```

The license suite uses ASan/UBSan, fresh test signing keys, the actual C++ verifier, a local PHP HTTP server with four workers, concurrent last-seat attempts, renewal/replay cases, corrupt stores and simulated short writes. Native callbacks use stubbed game functions. Separate tests cover production-runtime forged responses, superseded workers, typed-hook fallback and restoration on authorization loss. Java tests verify an OpenSSL-signed fixture and reject modified manifests, same-size binary modifications and truncation; they use a host Base64 adapter.

The ARM64 NDK build and compile of all Java sources against Android SDK 29 are checked separately. No Android gameplay session, hosted valid-key login, production APK signature, or real device suspend/network failure was available for end-to-end testing. Do not report those as verified.

## Limits

An attacker controlling a native process can patch checks, and shared source can be rebuilt with checks removed. IDA can still analyze a native library. The software-derived serial is not hardware attestation. This implementation raises the cost of panel/boolean/replay/download attacks; it does not make locally executed code uncrackable. See [ADR-001](decisions/0001-signed-license-sessions.md) for the trust model and trade-offs.

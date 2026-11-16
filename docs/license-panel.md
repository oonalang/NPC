# License and release protection

The client on `main` includes signed authorization and the corrected native character preview. It requires matching PHP files and a rebuilt launcher. The hosted installation has not been changed.

## Protocol

`POST https://ethnir.x10.mx/connect.php` accepts `protocol=2`, `game=CODMGR`, `build=1`, the normalized key, the existing device serial and a fresh 64-character lowercase hex nonce. Nonces contain 32 bytes from OpenSSL's CSPRNG. Requests use HTTPS certificate and hostname verification, system trust roots, no redirects and bounded connect/read/response sizes.

A successful JSON response contains `data.payload` and `data.sig`. The exact signed payload is:

```text
v2|CODMGR|1|key|serial|nonce|issued|lease_until|license_until|capabilities|key_id
```

RSA-SHA256 authenticates every field. Decimal values must be canonical. Unknown versions, audiences, builds, keys, devices, nonces, capabilities, key identifiers, invalid lifetimes and signatures fail closed. `key_id` is the SHA-256 fingerprint of the public key's DER SubjectPublicKeyInfo. Capability bit 1 grants menu access, bit 2 grants native features, and bit 4 identifies Premium. Free keys receive mask `3`; Premium keys receive mask `7`. Native UI derives the plan from the signed bit rather than the response's unsigned tier hint. Existing records without a tier retain their Premium interpretation.

Grants last at most 180 seconds, bounded by the license expiry. Revalidation runs on a worker after 60 seconds; transient failures retry after 10 seconds. A rejected or unverifiable renewal revokes the grant. An interrupted connection retains only the previous unexpired grant. Normal revocation is learned at renewal; network failures can delay it until that grant expires. No offline or v1 fallback exists.

Acceptance also records a deadline against `CLOCK_BOOTTIME`, including device suspend, bounded from the start of the request. Changing the phone's wall clock cannot extend an accepted grant. Pending generations and nonce matching reject replayed responses and late workers after a newer login. Neither finishing the loading animation nor assigning a local presentation flag creates a session. A verified renewal can resume an already opened menu after a connection interruption; an explicit new login resets the startup presentation. The device clock must be within 30 seconds of the signing server at verification.

## Native enforcement

`Security/Lease.h` constructs immutable grants only after verifying the signature and claims. `Security/LicenseRuntime.cpp` owns the active session and renewal worker. Native entry points consult authorization independently of menu presentation. Preferences remain separate from authorization.

Legacy memory hooks and the bullet/cosmetic runtime hooks use typed wrappers that forward arguments to the original function when authorization is absent. Hitbox, trigger input, slide fatigue and frame-rate overrides also enforce authorization within their native paths. Feature installation is deferred until signed login; capture-only skin/config collectors remain available during startup so existing catalog behavior is preserved.

Expired/revoked grants stop new overrides. FPS device fields, owned gravity/air-control values and active patches have restoration paths. In-flight calls may finish. Already spawned/applied cosmetics and every legacy persistent game-state change cannot all be reconstructed; this does not promise complete rollback of prior actions.

## Native download verification

The rebuilt launcher reads `https://ethnir.x10.mx/native/libZANEV1.manifest`. Before loading a cached file or installing a download from `/native/libZANEV1.so`, it verifies the publisher's signature, expected product/build/filename/size and SHA-256 hash. Redirects and other hosts are rejected. A partial download never replaces the active library; the candidate file is read-only and verified before an atomic rename. Failed verification does not load an unchecked cached file.

The release manifest consists of the following payload, newline, its canonical Base64 RSA-SHA256 signature, and a final newline:

```text
v1|SHAI_ETHNIR|1|libZANEV1.so|byte_length|sha256|key_id
```

The manifest is a release-integrity check, not a session grant or monotonic anti-rollback counter. Native session authorization remains mandatory. Existing installed launchers keep their old behavior until their APK is replaced.

The launcher wraps resource-managed reads in ordinary outer `try/finally` blocks for connection disconnection. AIDE reported two unexpected-`finally` errors when cleanup was attached directly to the resource-managed form, although standard Java accepts it. Keep that separation when editing the downloader. All launcher Java sources were compiled against the official Android 29 SDK with Java 8 source/target settings; the phone's AIDE parser itself was not available for verification.

## Deploy this build

1. Back up the live panel files, current signing key, `data/keys.json` and `data/giveaway.json`. Retain the live license records, device bindings and Linkvertise settings.
2. The current complete archive is `/workspace/license-hardening/artifacts/SHAI-ETHNIR-Updated-Full-Panel.zip`. It supersedes the earlier partial public/private update archives. It contains the user's full Claymorphism panel, updated PHP, protected server signing key, matching native library/manifest and deployment guides. Admin/free-page HTML, CSS and JavaScript are unchanged; `index.php` is byte-identical to the upload. Generated keys now use 128 bits of randomness.
3. Keep the entire ZIP private because it contains the supplied admin configuration and signing key. Extract locally and upload its contents into the existing panel root. The archive preserves supplied config values except `app_modulus`, which is replaced with the matching uppercase RSA modulus. It replaces the old private key with the RSA3072 key matching both clients. If hosting config changed after the upload was made, carry those path/credential edits forward. No live data JSON is included: do not delete or replace the hosted `data/` directory.
4. Check build `1` and the configured signing-key path, normally `private/licence_private.pem`. Restrict config/key/data access to the PHP account and verify Apache deny rules work on the host; prefer private paths outside the web root where supported. The new library uses a shared store lock for seat allocation, admin mutations and giveaway issuance, unique atomic temporaries, and rejection of malformed or short-written stores. Slow ad-provider requests do not hold the license-store lock.
5. Rebuild the launcher APK from this branch using your release signing certificate, then distribute it with the server update. Release builds no longer inherit the debug signing certificate. No production keystore or signed APK was supplied or generated here.
6. With a test key on an Android device, verify initial login, renewal past one minute, expiry/revocation, seat limits and denial of tampered releases. Both old clients and old panels need upgrading; protocol v1 receives HTTP 426. Uploading source alone does not update installed libraries or APKs.

Public giveaway keys still require three verified Linkvertise callbacks, last exactly 36,000 seconds from issuance and allow one device. Callback proofs must be distinct; fingerprints of claimed proofs are retained with the issued record to prevent reuse. Unfinished flows retain their 30-minute timeout, while completed claims remain visible until key expiry in the same browser session. The real provider verifier is preserved; test mocks never enter the deployment ZIP.

No private key is stored in git. Public trust roots are in `Security/LicenseRoot.h` and `LicenseRoot.java`. The current generated key is outside the checkout at `/workspace/license-hardening/keys/licence_private.pem`, and deployment archives are under `/workspace/license-hardening/artifacts/`. Back up the complete private archive securely; losing the private key requires another coordinated rotation.

For later rotations, generate a new private key outside the checkout with `python3 tools/license-keys.py --private-out /secure/new-signing-key.pem`, rebuild both native library and launcher, and redeploy the matching panel key. Do not run that command again for the already prepared binary.

To reproduce the complete package from the current uploaded panel using the same root:

```sh
python3 tools/package-current-panel.py \
  --source-panel /secure/ETHNIR-Claymorphism-3Ads-10Hours-FULL.zip \
  --library /path/to/libnets.so \
  --private-key /secure/signing-key.pem \
  --out /secure/SHAI-ETHNIR-Updated-Full-Panel.zip
```

This packager checks the audited UI, preserves config except the public modulus, checks Java/native/key agreement, verifies native hardening, signs/verifies the release, and excludes live databases. It retains the archive's private/data guards and includes no test server or vendor mock. `tests/current_panel_package_test.py` independently verifies the archive against the original ZIP, compiled library and public trust root, including PHP lint and unchanged credentials/settings.

For subsequent native-only releases on an already updated panel:

```sh
python3 tools/package-license-release.py \
  --library /path/to/libnets.so \
  --private-key /secure/signing-key.pem \
  --output-dir /path/outside/the/checkout
```

The native-only packager checks the source/binary/key match, verifies ELF hardening and creates separate public and private archives. It is not a full panel migration. Never publish any private archive as a downloadable release asset.

## Validation

Run with a PHP CLI that has OpenSSL and a JDK compiler:

```sh
sh tests/run-license-security-tests.sh
python3 tests/run-release-verifier-tests.py
sh tests/run-hitbox-tests.sh
sh tests/run-trigger-tests.sh
sh tests/run-frame-rate-tests.sh
sh tests/run-slide-fatigue-tests.sh
python3 tools/verify-native-hardening.py /path/to/libnets.so
```

The license suite uses ASan/UBSan, fresh test signing keys, the actual C++ verifier, a local PHP HTTP server with four workers, concurrent last-seat attempts, renewal/replay cases, corrupt stores and simulated short writes. The full panel HTTP tests additionally cover actual admin login/CSRF, Free/Premium batch creation, signed plan bits, legacy records, all three ad callbacks, exact ten-hour/one-device issuance, invalid/failed provider responses, repeated and cross-session proof reuse, retained completed claims and concurrent renewal during slow provider verification. Only the external vendor is mocked, in test-only PHP injected by the fixture. Native callbacks use stubbed game functions. Separate tests cover production-runtime forged responses, superseded workers, typed-hook fallback and restoration on authorization loss. Java tests verify an OpenSSL-signed fixture and reject modified manifests, same-size binary modifications and truncation; they use a host Base64 adapter.

The ARM64 NDK build and compile of all Java sources against Android SDK 29 are checked separately. No Android gameplay session, hosted valid-key login, real Linkvertise completion, production APK signature, or real device suspend/network failure was available for end-to-end testing. Do not report those as verified.

## Limits

An attacker controlling a native process can patch checks, and shared source can be rebuilt with checks removed. IDA can still analyze a native library. The software-derived serial is not hardware attestation. This implementation raises the cost of panel/boolean/replay/download attacks; it does not make locally executed code uncrackable. See [ADR-001](decisions/0001-signed-license-sessions.md) for the trust model and trade-offs.

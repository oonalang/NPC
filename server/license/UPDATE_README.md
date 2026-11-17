# SHAI ETHNIR full panel update

This is the complete updated version of your Claymorphism / 3 Ads / 10 Hours panel. Its admin interface, free-key page, credentials, license-store path and build setting are retained. The signing key and its public modulus have been updated together to match the included native library and the client source on `main` in `oonalang/NPC`.

**Keep this ZIP private. It contains your admin configuration and server signing key.** Extract it locally and upload the extracted files. Never leave the ZIP, backups, private keys or database downloads accessible through the website.

## Installation

1. Back up the existing panel, `config.php`, signing key, `data/keys.json` and `data/giveaway.json` privately. Existing keys, tiers, device bindings and Linkvertise settings must remain in those live JSON files.
2. Coordinate a short maintenance window with the client update. Upload this archive's extracted contents into the existing panel root, replacing files at the same paths. It includes `SessionProtocol.php`, protected `private/`, and the matching signed release in `native/`. It does not include or overwrite any live data JSON. Do not delete the existing `data/` folder. Check that the retained config still uses your hosted paths if you customized them after creating the original ZIP.
3. Restrict `config.php`, the private key and data to the PHP account. The `.htaccess` files block direct web access on Apache; confirm those rules work on the actual host. If the host does not honor them, place private files outside the web root and update their configured paths before enabling access. On other web servers, configure equivalent deny rules.
4. Rebuild and install the matching Android launcher from the updated source on `main` using your release signing certificate. The ZIP includes the ARM64 native library and its signed manifest, but no APK or production Android keystore. Updating the PHP or native library alone does not upgrade existing installed launchers.
5. The updated launcher uses `https://ethnir.x10.mx/native/libZANEV1.manifest` and `libZANEV1.so`. The license endpoint is `/connect.php` with `/connect` retained through Apache rewrite. Keep HTTPS enabled and the build at `1`.
6. Test a Free and Premium key on a device: initial login, renewal after one minute, expiry, revocation and device limits. Complete all three real Linkvertise checkpoints to verify your publisher configuration. No production server, real ad-provider request or Android gameplay session was used during preparation.

## Behavior

- Strict signed, nonce-bound v2 authorization replaces the older five/six-field responses. Old clients receive an update-required response. Do not mix old clients/panels with this release.
- Sessions last at most 180 seconds and renew after 60 seconds. A rejected renewal stops authorization; a network outage retains only an existing unexpired grant.
- Free/Premium status comes from signed permission bits. Free keys remain one-device keys. Older records without a tier retain their Premium interpretation.
- Public giveaway keys require three distinct verified Linkvertise proofs, expire exactly ten hours after issuance and allow one device. A completed claim remains visible for its key lifetime in the same browser session. Duplicate or already-claimed checkpoint proofs are rejected.
- Store updates use a shared lock and atomic replacement. Corrupt data is rejected rather than silently cleared. Ad verification does not hold the license-store lock while contacting Linkvertise.
- The rebuilt launcher verifies the signed native manifest and SHA-256 before loading the library. Both Java and native source must match the supplied signing root.

The included native build also extends adjustable-hitbox sizing to the supported sphere and capsule-as-box queries in addition to regular capsules. Direct hits retain native head/body classification and priority; no forced-headshot rule is added. General box queries and the game's broad-phase selection remain unchanged. Static ABI checks, host tests and the ARM64 build passed; these added geometry paths still require an in-game retest.

Game frame-rate options, including Ultra, are unlocked automatically once authorized on a compatible build. The mod no longer has a master unlock toggle, and an older saved disabled value cannot turn the unlock off. The separate numeric FPS overrides remain optional.

The native ESP preview now includes the parented Wiz rig, constrained hair/garment dynamics, articulated gestures and a playback clock that starts when the model is ready. Its artist attribution and noncommercial license are included in `native/WIZ_LICENSE.md`. Host GLES motion captures and source-rig tests passed; Android appearance and performance still require device testing.

The native trigger also recovers when a sniper's shot callback is missing, using verified clip consumption or a bounded cleanup deadline. Confirmation waits and duplicate updates skip redundant target traces, and periodic input checks avoid loader-list walks. Native aim/cooldown and fresh obstruction checks remain; phone testing is still needed to measure the reported missed shots and frame drops.

The shared trigger paths now skip unused ADS/screen-coordinate work and defer native readiness calls until they can affect input. Confirmed release-fire shots can rearm immediately when ready; confirmed press-fire releases avoid a redundant trace. Initial acquisition has no added reaction timer. Separate button edges, native weapon rules and existing unsupported-handler exclusions remain. Host timing/call-budget tests and the ARM64 build passed; no device FPS or reaction-time measurement is included.

Signed authorization raises the cost of panel swaps and response forgery. Software running on an attacker-controlled device can still be patched or rebuilt; this is not an uncrackable binary.

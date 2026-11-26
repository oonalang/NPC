# Protected library panel

The panel can publish a new `arm64-v8a` build without touching the APK. Upload a `.so` in the admin panel, and the launcher fetches it from a protected link that is only usable with a short-lived ticket signed by a key you control.

## What the owner sees

`libs.php` is reached from the **Libraries** button on the keys page and needs the normal panel login.

- Upload form: file, channel (default `lite`), optional note.
- Table of published builds with channel, size, SHA-256 prefix, upload time and a five-minute protected link you can test in a browser.
- The launcher download key, shown once and copyable, with a **Regenerate** button.
- Delete per build, which also removes its manifest and metadata.

Each upload is stored under its own name, so a `libnets.so` upload becomes `libnets.so` and a link looks like:

```
https://ethnir.x10.mx/lib/protected/libnets.so?e=1790000000&s=<64 hex>
```

Uploading the same name again replaces that build and its signed manifest, so the published link keeps working. A file name that still carries an old tag (for example `libnets.0a656191.so`) is cleaned back to `libnets.so` on upload. The stored name is what the launcher pins the manifest to.

## Upload checks

A file is refused unless it is a little-endian ELF64 with `e_machine` `0xB7` (AArch64), between 1 byte and 64 MiB. The manifest is then signed with the licence RSA key:

```
v1|ETHNIR_LITE|1|<stored file>|<bytes>|<sha256>|<key_id>
```

`key_id` is the SHA-256 of the public key's DER `SubjectPublicKeyInfo`, exactly as `tools/sign-native-release.py` and `VerifiedLibrary.java` expect. `library_product` and `library_build` in `config.php` override the product and build fields; the defaults are `ETHNIR_LITE` and `1`.

## Endpoints

| Route | Method | Purpose |
| --- | --- | --- |
| `/lib/token` | POST | Signs out a ticket for the newest build on a channel |
| `/lib/protected/<file>` | GET + token | Streams the library |
| `/lib/protected/<file>.manifest` | GET + token | Streams the signed manifest |

Ticket request fields: `file` (`latest` or a name), `channel`, `ts` (unix seconds), `nonce` (32 hex), `sig`. The signature is

```
HMAC-SHA256(key, "v1|" . file . "|" . channel . "|" . ts . "|" . nonce)
```

with a ±120 second clock window. The reply carries `file`, `length`, `sha256`, `path`, `e`, `s`, `manifest_path`, `manifest_e`, `manifest_s`. Download tokens are HMAC-SHA256 over `v1|<name>|<expires>` and live five minutes.

## Launcher side

`MainActivity` mints a ticket, downloads the manifest and checks it with `VerifiedLibrary`, downloads the library through the protected path, compares size and SHA-256, swaps the file in atomically and only then calls `System.load`. The cached file is reused when it still matches the manifest.

The launcher holds the download key in `SECRET`. Copy the value from the panel into that constant, rebuild the APK, and the same APK keeps working across library uploads because it always asks for `latest` on its channel. Rotating the key invalidates every installed launcher until it is rebuilt.

The key is used exactly as the panel prints it: 64 ASCII hex characters fed to `Mac` as bytes. Hex-decoding it into 32 raw bytes produces a different HMAC, so every ticket request comes back `403 Rejected`.

### What the dialog says

The download task keeps the failures apart instead of collapsing them into one line. Each case has its own dialog text:

| Dialog | Meaning | Fix |
| --- | --- | --- |
| This launcher has no download key. Rebuild it with the key from the panel. | `SECRET` is empty or not 64 characters | Copy the key and rebuild |
| The panel rejected this launcher's download key. Check the phone's date and time and rebuild with the key shown on the panel. | Ticket POST answered `401`/`403`, or the reply carried `status: false` | Match `SECRET` to the panel and keep the phone clock within two minutes of the host |
| The panel has no published build for this channel. Upload a library in the panel and try again. | Ticket POST answered `404` | Upload a signed build on the launcher's channel (`lite` by default) |
| The build is signed with a key this launcher does not trust. Rebuild it with the signing key shown on the panel. | Manifest `key_id` or RSA signature does not match `LicenseRoot` | Copy `KEY_ID` and `PUBLIC_KEY` from the panel's "Launcher signing key" section |
| Could not reach the release server. Check your connection and try again. | DNS, connect, read timeout or TLS failure | Retry on a working connection |
| The release server did not answer with a build. Try again later. | Any other HTTP status or a reply that is not ticket JSON | Check the host, the rewrite rules and the PHP error log |
| Build signature or contents could not be verified. | Manifest framing, claims or SHA-256 mismatch | Re-upload the library, or rebuild when the signing key pair changed |

A ticket rejection and an unreachable host are therefore separate answers: the first points at `SECRET` or the clock, the second at the network. A wrong signing key is reported on its own, without downloading the whole library again.

`VerifiedLibrary.read(bytes, fileName)` takes the expected name, which is the stored file name. The launcher also names its cache file after the ticket's `file` field, so whatever name is uploaded on the panel (`libnets.so`, for example) is the name that is downloaded, verified and loaded; there is no local name to keep in sync.

## Deploy

0. Upload the whole panel folder, then open `setup.php` once. The panel needs `config.php` beside `admin.php`; when that file is missing every page stops with a blank 500. `setup.php` writes it, reuses `private/licence_private.pem` when it is already there, prints the public key ID for the APK, and locks itself while `config.php` exists.
1. Upload `libs.php`, `libstore.php`, `libdl.php` and `libtok.php` next to `admin.php`, and replace `.htaccess`.
2. The host must allow these rewrites in the panel root:

```
RewriteRule ^lib/protected/([A-Za-z0-9][A-Za-z0-9._-]{0,79}\.so(?:\.manifest)?)$ libdl.php?f=$1 [L,QSA]
RewriteRule ^lib/token/?$ libtok.php [L]
```

   `libtok.php` and `libdl.php` must stay out of the `.htaccess` deny list. `mod_rewrite` maps `/lib/token` onto `libtok.php` and `/lib/protected/<file>` onto `libdl.php`, and `<FilesMatch>` is evaluated against the rewritten name, so denying those two files turns every protected route into a host-level `403` page. Both validate an HMAC before doing anything, exactly like `/connect`, so leaving them reachable directly gives nothing away.

3. `private/libs/` is created on first upload with its own deny rule; `private/lib_secret` (0600) is created the first time the page needs it. Both stay out of the web root on hosts that allow it.
4. Raise `upload_max_filesize` and `post_max_size` if a build is rejected; a 4 MiB library needs at least 8 MiB of `post_max_size`.
5. Copy the key into `MainActivity.SECRET`, rebuild the launcher, and test one upload end to end before publishing the APK.

## Key rotation

Two values are pinned in the APK:

- `MainActivity.SECRET`, the download key from `private/lib_secret`;
- `LicenseRoot.PUBLIC_KEY` and `LicenseRoot.KEY_ID`, the RSA key that signs manifests.

Replacing the panel folder with a fresh copy instead of uploading over the existing one deletes `private/` and `data/`. The next page load then creates a new `private/lib_secret`, and a new signing key is created whenever `private/licence_private.pem` is missing. Both pinned values stop matching at once: the ticket POST answers `403`, and once the ticket is right, manifest verification fails.

The panel prints the current values under "Launcher download key" and "Launcher signing key". Copy both into the source and rebuild. Uploading the panel files over the existing install keeps `private/` and `data/` intact and needs no rebuild.

## Limits

- The key ships inside the APK, so an attacker who extracts it can mint tickets. The ticket only grants a download; the signed manifest still decides what the device is willing to load.
- Deleting a build leaves already-downloaded copies on phones; they keep loading until the next ticket resolves to a different build.
- The panel signs with the same RSA key that issues licences. Losing that key requires rotating the licence root and rebuilding both clients.

## Validation

`sh tests/run-library-panel-tests.sh` (or `PHP=/path/to/php sh tests/run-library-panel-tests.sh`) copies the panel into a temporary directory, then checks:

- tagging, digest recording, manifest payload and RSA signature;
- rejection of non-ELF, 32-bit, non-arm64, oversized files and an unusable key;
- ticket expiry, wrong file, oversized lifetime and forged signatures;
- `latest` resolution per channel;
- the real HTTP endpoints: valid download, missing ticket, forged ticket, manifest download, ticket issue, unsigned ticket request;
- the admin page: login form, signed-in page, multipart upload, anonymous refusal.

It needs the PHP CLI with `openssl`, `session` and `json`, plus `curl`. It prints `SKIP` when either is missing.

The suite runs on `php -S`, which ignores `.htaccess`, so the rewrite and deny rules themselves are not covered. Check them on the host: `POST https://<host>/lib/token` must answer JSON (`405` or `403` with a `reason`), never a host error page, and `/lib/protected/<file>` without a token must answer `403` from the panel, not from Apache.

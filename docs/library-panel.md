# Protected library panel

The panel can publish a new `arm64-v8a` build without touching the APK. Upload a `.so` in the admin panel, and the launcher fetches it from a protected link that is only usable with a short-lived ticket signed by a key you control.

## What the owner sees

`libs.php` is reached from the **Libraries** button on the keys page and needs the normal panel login.

- Upload form: file, channel (default `lite`), optional note.
- Table of published builds with channel, size, SHA-256 prefix, upload time and a five-minute protected link you can test in a browser.
- The launcher download key, shown once and copyable, with a **Regenerate** button.
- Delete per build, which also removes its manifest and metadata.

Each upload is stored as `<original-name>.<8-hex-tag>.so`, so a link looks like:

```
https://ethnir.x10.mx/lib/protected/libZANEV1.4f9c2a71.so?e=1790000000&s=<64 hex>
```

The tag keeps two uploads of the same build from overwriting each other; the file name is what the launcher pins the manifest to.

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

`VerifiedLibrary.read(bytes, fileName)` takes the expected name, because the published build carries a tag. The launcher also names its cache file after the ticket's `file` field, so whatever name is uploaded on the panel (`libnets.so`, for example) is the name that is downloaded, verified and loaded; there is no local name to keep in sync.

## Deploy

0. Upload the whole panel folder, then open `setup.php` once. The panel needs `config.php` beside `admin.php`; when that file is missing every page stops with a blank 500. `setup.php` writes it, reuses `private/licence_private.pem` when it is already there, prints the public key ID for the APK, and locks itself while `config.php` exists.
1. Upload `libs.php`, `libstore.php`, `libdl.php` and `libtok.php` next to `admin.php`, and replace `.htaccess`.
2. The host must allow these rewrites in the panel root:

```
RewriteRule ^lib/protected/([A-Za-z0-9][A-Za-z0-9._-]{0,79}\.so(?:\.manifest)?)$ libdl.php?f=$1 [L,QSA]
RewriteRule ^lib/token/?$ libtok.php [L]
```

3. `private/libs/` is created on first upload with its own deny rule; `private/lib_secret` (0600) is created the first time the page needs it. Both stay out of the web root on hosts that allow it.
4. Raise `upload_max_filesize` and `post_max_size` if a build is rejected; a 4 MiB library needs at least 8 MiB of `post_max_size`.
5. Copy the key into `MainActivity.SECRET`, rebuild the launcher, and test one upload end to end before publishing the APK.

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

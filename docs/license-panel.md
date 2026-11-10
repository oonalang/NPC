# License panel integration

The login screen posts `game=CODMGR`, the license key, and its existing device
identifier to `https://ethnir.x10.mx/connect.php` over HTTPS. The panel binds a
new device to an available seat when it accepts the key. Each login requires a
network request; the locally saved key is a convenience, not offline access.

The panel source supplied with this integration signs `v1|key|serial|expiry|1`
using RSA-SHA256. The client verifies the signature and every field against the
request and response before accepting it. The public verification key is embedded
in `src/main/jni/Includes/PanelLicense.h`. Deploy the same panel signing key as
the supplied source; if that key is rotated, rebuild the client with its new public
key. Never place the private signing key in the app or this repository.

The panel's signed response contains neither a nonce nor a short-lived issuance
time. A captured response for the same key and device can therefore be replayed
until its expiry, and an already running session does not immediately learn about
server-side revocation. Addressing that requires a panel protocol change and
ongoing revalidation. On local expiry the login screen returns and the trigger
is disabled; other already-installed hooks and patches are not fully revoked.

## Delivery and verification

`MainActivity.java` currently downloads `libZANEV1.so` from a separate host.
Rebuilding this source alone does not update that hosted file or existing
installations. Build the ARM64 library and publish the resulting library under
the launcher’s configured filename and URL, or update the launcher’s distribution
configuration. Publishing requires access to that hosting account.

The signed-response verifier can be checked on a development machine with OpenSSL:

```sh
c++ -std=c++17 -Wall -Wextra -Werror -fno-exceptions \
  tests/panel_license_test.cpp -lcrypto -o /tmp/npc-panel-license-test
/tmp/npc-panel-license-test
```

Before release, use a test license generated in the hosted panel on an Android
device. Check valid login, wrong key, an expired or revoked key, and the device
seat limit. A valid test is also needed to confirm that the hosted panel is using
the same signing key as the supplied source. The public endpoint rejects empty
credentials, but no valid hosted license or live Android session was available
for this integration.

# Home, Wiz and authenticate-flow fixes

Home stays first. The music source editor now has a centered, opaque dialog and an integrated touch keyboard. Paths support lowercase letters, slashes, punctuation and spaces, plus paste, clear and UTF-8 deletion. ImGui's hidden-label inputs use their requested width instead of the old half-row layout. Home measures the taller Cloud card and scrolls the dashboard without a visible scrollbar when it does not fit.

Cloud has a search field for Spotify and YouTube Music. Search opens the selected service's app, with a web fallback. Account sign-in and playback happen in that service. The transport buttons send Android media commands to the active session; they do not establish an account connection or report verified provider playback state. No Premium-only Spotify player or undocumented YouTube Music audio endpoint is embedded.

The ESP preview uses Wiz rendered from the exact Glayer3d model the user supplied. Its existing breathing, hair, jump/cheer and secondary-motion animation now uses anchors adjusted to Wiz. The rendered PNG stays cached; there is no FBX import or frame-by-frame image decoding on Android. Attribution, source hashes, the noncommercial license and reproduction instructions are in [WIZ_LICENSE.md](src/main/jni/Assets/WIZ_LICENSE.md).

The weapon configuration layout now matches the provided 1.6.59 dump: `IsDouble` is at `0x44`, the mount type at `0x48`, color text at `0x50`, camo data at `0x58`, description at `0x60`, and name at `0x68`. Name lookup runs once before applying a skin and skips a missing name. The previous layout targeted camo data when trying to change the name.

Authentication now uses the offscreen refraction compositor in the actual Android render path. The card and all glass callbacks are composited together before the suck-in transform. The shrinking card stays clear of viewport edges, and the explosion uses a continuous irregular front, bounded displacement, RGB source separation and a transparent warm interior. Production preserves the current game backdrop; the GPU harness uses a checkerboard to expose refraction. The old drawn explosion outlines were removed. The sign-in glyph is visible again, the button retains its full pill frame, and the fluid has an explicit clip rectangle in addition to its curved wave geometry.

All existing named phase durations stay in `StartupState.h`. The calm interval is 8050–8400 ms; loading reveals at 8400 ms and settles at 8850 ms. The extra 750 ms is the settled review hold, ending at 9600 ms.

## Verified

- `tests/run-music-library-tests.sh`: full-width input geometry, modal placement, hardware typing, keyboard edits, actual touch opening/key presses, UTF-8 deletion, search URL encoding, library scanning, snapshots and worker state under ASan/UBSan.
- `tests/run-home-layout-tests.sh`: Local/Cloud and error states across wide, short landscape, portrait and narrow viewports at two UI scales. Controls fit their card; overflow remains reachable without a visible dashboard scrollbar.
- `tests/run-weapon-fields-tests.sh`: the name write at `0x68` leaves the camo and description pointers intact, plus compile-time layout assertions.
- `tests/run-startup-tests.sh`: phase durations, success/reduced-motion gates, delayed loading, centering, touch blocking and startup actions.
- `tests/run-anime-preview-tests.sh`: animation geometry, spring bounds, jumping, finite vertices, overlays and the embedded Wiz PNG contract.
- `AUTH_REFERENCE_GIF=/path/to/chsiVbH.gif tests/run-auth-storyboard-gpu-tests.sh /tmp/auth-review`: GLES rendering, whole-card collapse containment, irregular front, transparent interior, bent checker edges, RGB source separation, 41,175 exterior fluid comparisons, GL state restoration, live framebuffer capture and off-center propagation. At 60 sampled frames per second the calm beat is exactly 21 frames, reveal starts at 8.400 s and settles at 8.850 s. Captures and a phase-aligned reference comparison are emitted by the script.
- Native GLES previews: actual embedded Wiz PNG uploaded once and reused during motion; Home and music dialogs rendered for visual inspection.
- ARM64 NDK r22 build: `libnets.so`, Android API 21. No explanatory source comments were added.

## Android checks still required

1. Equip ZRG 20mm / Ticket to Hell, leave blueprint selection and Gunsmith, then inspect the loadout and weapon names. Host tests validate the corrected fields, not the game's caches or server state.
2. Open the folder editor from Home, enter an accessible music folder with the touch keyboard, scan it, play, seek and return to the game. Shared storage access depends on the host app's permissions.
3. Search in each cloud provider, sign in in its app, start music and return to the game. Check play/pause and previous/next against the active media session, including the user's Spotify Free account. Actual results and playback are service-controlled.
4. Run authentication with a moved login card and at the device's landscape aspect ratio. Watch the card edges, both sides of the shockwave, the calm beat and the staggered reveal. Measure device FPS, GPU time and memory. Software Mesa render/readback timings are not Android performance measurements.
5. Exercise reduced motion, GL context recreation, app background/resume and license-expiry return to login. No Android device was available for this pass.

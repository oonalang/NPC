# Home and animated ESP preview: next steps

Handoff state after the layout, player-state and build passes. Home is the first navigation slot with session metrics, a performance graph and a compact music player. The ESP preview uses an original adult character with mesh-based breathing, hair motion, jumps and secondary movement, and its overlays use the animated character bounds.

## Finished in this pass

- Compact Home layout, checked headless at 900x520, 700x420, 620x300, 1000x300, 420x760, 330x600 and 280x520 with `tests/run-home-layout-tests.sh` at two UI scales in local, error, cloud and cloud-error states. The music card now sizes itself to its measured content, so the Local and Cloud controls are never clipped, and the dashboard panel scrolls only when the stack genuinely does not fit.
- Music library, player service and cloud-mode state: `tests/run-music-library-tests.sh`.
- ESP preview animation, mesh indices, weights and animated bounds, plus the embedded artwork format: `tests/run-anime-preview-tests.sh`.
- ARM64 build: `ndk-build` with NDK 22.0.7026061 (r22b), `APP_ABI=arm64-v8a`, produces `libs/arm64-v8a/libnets.so`, including `Assets/AnimeArtwork.S` with the `adult-preview.png` blob.
- Player and artwork worker lifetimes, JNI error paths and playback-state reporting reviewed. The music player now attempts `MediaPlayer` setup once instead of retrying every poll tick, a scan that runs before the music folder resolves no longer reports a bogus folder error, and the Cloud play/pause icon follows the last cloud command instead of `isMusicActive`, which also drops the four-per-second binder poll. `isMusicActive` still cannot prove that Spotify or YouTube Music is playing, because it includes game audio.
- No explanatory comments were added to the touched sources during this work.

## Still needs an Android device

- Spotify and YouTube Music launch and media controls, including what the active music app does with `dispatchMediaKeyEvent`. Cloud controls operate the active music app; they do not embed either service's catalog.
- Local folder selection, scanning, playback, seeking and error reporting, including scoped-storage permissions and `MediaPlayer` behavior after audio focus loss.
- The GL texture upload path in `AnimeArtwork::Ensure` (host builds have no GLES/EGL headers), the full ESP preview with all overlays while jumping, and the animated bounds against a real target.
- Actual device frame rate, audio behavior, graphics context recreation and memory use. The embedded artwork uses approximately 6 MiB for decoded pixels plus 6 MiB for the GPU texture. The decoded pixels are deliberately retained so the texture can be re-uploaded after context recreation.

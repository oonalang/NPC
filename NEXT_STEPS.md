# Home and animated ESP preview: next steps

This is a work-in-progress handoff. Home was moved to the first navigation slot and rewritten with session metrics, a performance graph, and a compact music player. The ESP preview uses an original adult character with mesh-based breathing, hair motion, jumps, and secondary movement. ESP overlays use the animated character bounds.

- Finish checking the compact Home layout at landscape and narrow sizes. Ensure Local and Cloud music controls remain visible and usable without excessive scrolling.
- Verify Spotify and YouTube Music launch and media controls on an Android device. Cloud controls operate the active music app; they do not embed either service's catalog.
- Verify local folder selection, scanning, playback, seeking, and errors on Android, including scoped-storage permissions.
- Complete the ARM64 NDK build and run the native dashboard, music-library, texture-loader, and animation checks. Inspect the full ESP preview with all overlays enabled and while jumping.
- Test actual device frame rate, audio behavior, graphics context recreation, and memory use. The embedded artwork uses approximately 6 MiB for decoded pixels plus 6 MiB for the GPU texture.
- Review asynchronous player and artwork worker lifetimes, JNI errors, and playback-state reporting before release. Android's `isMusicActive` can include game audio, so it must not be treated as proof that Spotify or YouTube Music is playing.
- Remove any comments added during this work if present, while retaining required license notices.

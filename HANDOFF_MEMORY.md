# Compact handoff

- Active worktree: `/workspace/NPC-hitboxes`; branch `feature/adjustable-hitboxes`; based on main `306e6cf` (Keep the Wiz animation render sheets).
- Current request: adjustable enemy hitbox size. Preserve ordinary head/body classification. User explicitly does **not** want always-headshots. They supplied two reference libraries in [Drive](https://drive.google.com/drive/folders/1L8BHYh1C7b3n8mSClkf7oLKheLwZqjIu).
- VEL's slider really scales collider dimensions for a query. NPC's old toggle just returned true. New branch replaces that shortcut with normal-first, then scaled detailed collision queries. Size is 1–5×, default 1×, under Memory → Weapon handling. JSON/INI persist it.
- Uses original collider objects and inverse-scaled rays; no shared collider mutations, hit-group rewrites, aim redirection or damage changes. Local owner/enemy membership required. Thread-local scope and bounded lists.
- Pinned game ELF: `/workspace/dumps/libunity.so`, build ID `056d6390cc712865`, version 1.6.59. Four encoded methods must decode before hook installation. Readiness heuristics are not full decoded-code/ABI verification.
- Broad-phase selection/range still limit which targets can be hit. Actual server damage and performance need device testing. No Android gameplay verification has been performed.
- Read [the detailed handoff](docs/ADJUSTABLE_HITBOX_HANDOFF.md) for evidence, addresses, test coverage, artifact location and device checklist. Run `sh tests/run-hitbox-tests.sh`, `sh tests/run-memory-tab-tests.sh`, `sh tests/run-trigger-tests.sh`, and `python3 tests/verify-hitbox-profile.py /workspace/dumps/libunity.so`.
- Android build/artifacts live outside git at `/workspace/hitbox-reference/`. Do not add the reference `.so` files or tracked build-object changes to a commit.
- Prior Wiz worktree `/workspace/NPC-liquid-ui`, branch `feature/esp-3d-runtime-physics`, remains preserved separately. Do not overwrite main's newer Wiz changes with it during this task.
- User prefers direct action, feature work for review, no added source comments, and candid reporting of unexecuted checks. The latest request did not authorize a new main push.

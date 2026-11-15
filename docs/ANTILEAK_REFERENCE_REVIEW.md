# Antileak reference review

## User request

Add the behavior described as antileak, compare the attached `kick.py` with the Eclipse reference, and resolve old reference addresses against the current game dump before porting anything. The outstanding clarification is whether antileak means protecting the active CODM login or preventing mod-license sharing.

## What the Python file actually does

Source inspected: `/workspace/.codex-attachments/ce2cfd4b-d21c-4f17-8c1f-22705a2cf768/1d31ccfe-ffed-4a71-8dd2-785cb32f72b8/kick.py`. It was parsed and read, not executed.

The 190-line script offers Garena/Global package selection, one-time launch, repeated launch, foreground monitoring and optional delayed screen taps. It invokes Android's `monkey`, `am`, `input`, `dumpsys` and `pm` commands through subprocesses.

It has no native game offsets, login/session API, server authentication request, account identifier, device binding or license-sharing enforcement. The claim in its title/comments that other logins will be kicked is not implemented by the code. Reopening an app does not establish an exclusive authenticated session.

Additional porting concerns: its Watch mode checks foreground focus rather than process liveness; its pending tap can still execute after Stop; worker threads call Tk UI methods directly; and command availability does not establish the Android permissions required to use it. A native library inside CODM cannot act as an external watchdog after CODM's own process exits.

## Current dump identity

The supplied [dump dataset](https://huggingface.co/datasets/jeudhehs/dump/tree/main) currently contains version 1.6.59. Its observed repository revision is `60f0056cd45d4efdf161f11e40fe03ad6ab943d0`. Local files exactly match the remote LFS SHA-256 values:

| File | SHA-256 |
| --- | --- |
| Dump 1.6.59.cs | `f370b52ac099770074b888a745e0d255407c6ea6d5314f10561a10e2927954b6` |
| libunity.so | `4e2e723da70deefb2cb986375645ae2fcab47343472cb7fdfcb49077b4919111` |

The game ELF's GNU build ID is `056d6390cc712865`. This establishes the version of the supplied reference data, not the version installed on an uninspected phone.

## Address migration status

Addresses inside a mod library identify the mod's own functions and data. They are different from RVAs inside the game's `libunity.so`. Porting requires identifying a game's method, signature and data layout first, then resolving it in the matching game build. Old mod addresses cannot be reused as current game offsets.

| Reference | Identified behavior | Current mapping/status |
| --- | --- | --- |
| VEL Hitbox | Temporary collider-size scaling around a weapon query; adjustable 1–5× | NPC's new implementation uses 1.6.59 game RVAs and verified layouts, documented in ADJUSTABLE_HITBOX_HANDOFF.md |
| Eclipse Extend Hitbox | UI control located | Its underlying callback has not been fully established; no guessed port added |
| Eclipse antileak | Not positively identified in the supplied libECLIPSESKIN.so | No claim that its addresses or behavior have been recovered |
| kick.py | Android app launch/relaunch and optional taps | No game offsets to update |

Neither supplied library yielded a plain antileak/session-lock control string. That is not proof the functionality is absent: it could be encoded, unconditional, implemented elsewhere, or present in another build. Imports and strings alone do not establish behavior.

## Current-game session-control candidate

The dump contains `Network.ForbidKickOffHandler.ForbidKickOff(bool forbid)` at game RVA `0xC977A24` (dump line 1621725). Its first 16 disk bytes are:

```text
ff c3 00 d1 fe 57 01 a9 f4 4f 02 a9 f4 5a 01 b0
```

The native code is readable in this ELF. Disassembly confirms that it receives the boolean in `w1`, masks it to a boolean, writes an integer state at request-object offset `0x18`, and submits that object through the game's networking path. This follows the static-method receiver slot used by this game; declaring it as a function taking only one boolean would give the wrong ABI.

The dump also defines `CSClientForbidKickOffReq`, an integer `state`, enable/disable enum values and request/response protocol constants. These are corroborating structural clues, not proof that the server grants exclusive login ownership or that Eclipse invokes this method. No network request was sent and no live account state was changed during this review.

## Implementation status and next step

No antileak toggle, account-session patch or launcher loop has been added. The reference behavior and intended protection must be established before choosing an implementation. Do not implement this by silently suppressing a logout message and claiming that the server session remains valid.

If the user means CODM login protection, establish the normal lifecycle and server response of the existing session-control mechanism and identify Eclipse's actual invocation, if present. A launcher loop is a separate feature and cannot supply that guarantee. If the user means mod-license sharing, enforcement belongs in the license service with device/session policy rather than game offsets.

The prior hitbox feature remains committed as `c04aa18` on `feature/adjustable-hitboxes`. Both source comments and unrelated functionality remain unchanged by this review.

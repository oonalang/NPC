# CODM 1.6.59 offset comparison

Compared the NPC source against the supplied `Dump 1.6.59.cs` metadata and the third-party offset list. The metadata shows field layouts and method RVAs, not proof of runtime safety, hook signatures or a compatible live binary.

## Confirmed repairs

| Definition | Earlier NPC value | Dump value | Evidence |
| --- | --- | --- | --- |
| `Class_PlayerInfo_m_NickName` | `0x160` | `0x170` | `GameEngine.PlayerInfo.m_NickName`, dump line 1,079,021 |
| `Class_PlayerInfo_m_ProfessionChipID` | `0x1E8` | `0x208` | `GameEngine.PlayerInfo.m_ProfessionChipID`, dump line 1,079,053 |
| `Class_BRDroppedPickUpNormal_ItemConfigType` | `0xB0` | `0xA0` | `OpenWorldDroppedPickUpNormal.ItemConfigType`, dump line 1,647,862 |
| `Class_LethalAndTacticalConfConfig_LOCID_Name` | `0x68` | `0x58` | `LethalAndTacticalConfConfig.LOCID_Name`, dump lines 947,087–947,123 |
| `Class_LethalAndTacticalConfConfig_MarkOfName` | `0x50` | `0x58` | This legacy alias refers to `LOCID_Name`, not `LOCID_Desc` |

`StructSDK.h` previously read the legacy weapon and throwable-name fields as `String*`, including using the throwable alias for weapon names. Both fields are `HalfString` references in the dump. The accessors now use their respective name offsets and expose incomplete `HalfString*` pointers instead of lying about the type. They should not be interpreted as ordinary `String*` when decoding text.

## Values already correct in NPC

| Definition | Value | Source evidence |
| --- | --- | --- |
| `Class_Pawn_m_PlayerInfo` | `0x5C8` | `Pawn.m_PlayerInfo`, dump line 840,027 |
| `Class_Pawn_m_IsBot` | `0x5C1` | `Pawn.m_IsBot`, dump line 840,026 |
| `Class_Pawn_m_IsAlive` | `0x550` | `Pawn.m_IsAlive`, dump line 839,999 |
| `Class_Pawn_m_HeadBone` | `0x310` | `Pawn.m_HeadBone`, dump line 839,900 |
| `Class_Pawn_m_Mesh` | `0x630` | `Pawn.m_Mesh`, dump line 840,042 |
| Spine, neck, hips and ankle bones | `0x1E18`–`0x1E38` | Pawn bone fields, dump line 852,442 and following |
| `Class_MatchGame_get_DroppedPickUpMgr` | `0x5BDC4C8` | Instance getter, dump line 838,053 |
| `Class_BRDroppedPickUpNormal_mDroppedPickUpConfig` | `0xD8` | `OpenWorldDroppedPickUpNormal.mDroppedPickUpConfig`, dump line 1,647,874 |
| `Class_BRDroppedPickUpNormal_mWeaponConf` | `0xE0` | Same class |
| `Class_BRDroppedPickUpNormal_mThrowConf` | `0xE8` | Same class |
| `Class_BRDroppedPickUpMgr_mNormalItemEntityDic` | `0x118` | Inherited `OpenWorldDroppedPickUpMgr.mNormalItemEntityDic`, dump line 1,648,836 |
| `Class_WeaponConfConfig_LOCID_Name` | `0x68` | `WeaponConfConfig.LOCID_Name`, dump around line 958,700 |
| `Class_Physics_Raycast` | `0x531D9E8` | `Physics.Raycast(Vector3,Vector3,float,long)`, dump line 277,620 |

The friend's suggested `0x138` normal-item dictionary value is **incorrect for this dump**. `0x138` is `OpenWorldDroppedPickUpMgr.mSpawnDic`, not `mNormalItemEntityDic`. NPC correctly retains `0x118`.

The friend's throwable name `0x50` also does not match `LOCID_Name`: `0x50` is `LOCID_Desc`; the actual name is `0x58`.

## Live BR item tracking

Existing `WorldESP.h` already uses these dump-backed normal-pickup field offsets:

- `OpenWorldDroppedPickUpBase.ItemData` at `0x50`, `ItemID` at `0x5C`, `mServerPos` at `0x60`
- `OpenWorldDroppedPickUpData.ItemType` at `0x10`, `ItemID` at `0x1C`, `Count` at `0x20`, `IsInBox` at `0x30`, `Pos` at `0x4C`
- `OpenWorldDroppedPickUpNormal.ItemConfigType` at `0xA0`, `Count` at `0xAC`, `IsInBox` at `0xC1`, `mDroppedPickUpConfig` at `0xD8`

The existing pickup hook addresses also match dump method declarations: normal `Tick(float)` at `0x64FA604`, BR `Tick(float)` at `0x8A06D60`, and `OpenWorldDroppedPickUpBase.OnEnable/OnDisable` at `0x94DE944/0x94DE990`.

These matching declarations **do not** establish that the hooks land, the item manager publishes entries, the filters accept them or ESP labels appear in-game. Use the `Tracked pickups`, `Loot callbacks`, `Accepted`, `Rejected` and `Labels` diagnostics already added to the experimental branch.

## Unresolved / left unchanged

- `Class_DroppedPickUpConfig_configID`: the source says `0x68`, the third-party snippet says `0x10`. No unambiguous matching `DroppedPickUpConfig.configID` declaration was found, so neither value is safe to assert as verified. Left unchanged.
- The native binary build ID and target device runtime were not confirmed in this comparison. Matching a dump's numeric RVA alone does not guarantee cross-build compatibility.
- No claim of an in-game BR Item ESP fix or measured FPS gain is made without an ARM64 build and runtime observation.

Run `python3 tests/verify_1659_offsets.py` to check the known field definitions and relevant existing hook addresses.

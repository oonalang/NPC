from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
definitions = (root / "src/main/jni/Hacks/StructGame/Defines.h").read_text()
sdk = (root / "src/main/jni/Hacks/StructGame/StructSDK.h").read_text()
world = (root / "src/main/jni/System/Hooks/WorldESP.h").read_text()
hooks = (root / "src/main/jni/System/Hooks/Feats.h").read_text()

expected = {
    "Class_Pawn_m_PlayerInfo": "0x5C8",
    "Class_Pawn_m_IsBot": "0x5C1",
    "Class_Pawn_m_IsAlive": "0x550",
    "Class_Pawn_m_HeadBone": "0x310",
    "Class_Pawn_m_Mesh": "0x630",
    "Class_PlayerInfo_m_NickName": "0x170",
    "Class_PlayerInfo_m_ProfessionChipID": "0x208",
    "Class_MatchGame_get_DroppedPickUpMgr": "0x5BDC4C8",
    "Class_BRDroppedPickUpNormal_ItemConfigType": "0xA0",
    "Class_BRDroppedPickUpNormal_mDroppedPickUpConfig": "0xD8",
    "Class_BRDroppedPickUpNormal_mWeaponConf": "0xE0",
    "Class_BRDroppedPickUpNormal_mThrowConf": "0xE8",
    "Class_BRDroppedPickUpMgr_mNormalItemEntityDic": "0x118",
    "Class_WeaponConfConfig_LOCID_Name": "0x68",
    "Class_LethalAndTacticalConfConfig_LOCID_Name": "0x58",
    "Class_LethalAndTacticalConfConfig_MarkOfName": "0x58",
    "Class_Physics_Raycast": "0x531D9E8",
}

problems = []
for name, expected_value in expected.items():
    match = re.search(r"^\s*#define\s+" + re.escape(name) +
                      r"\s+\(?\s*(?:m_unity\s*\+\s*)?(0x[0-9a-fA-F]+)\s*\)?",
                      definitions, re.MULTILINE)
    if not match or int(match.group(1), 16) != int(expected_value, 16):
        problems.append(f"{name}: expected {expected_value}, got {match.group(1) if match else 'missing'}")

fields = {
    "ItemData": "0x50",
    "ItemType": "0x10",
    "ItemId": "0x1C",
    "Count": "0x20",
    "IsInBox": "0x30",
    "Position": "0x4C",
    "Config": "0xD8",
    "ServerPosition": "0x60",
    "NormalItemType": "0xA0",
    "NormalCount": "0xAC",
    "NormalInBox": "0xC1",
    "BaseItemId": "0x5C",
}
for name, expected_value in fields.items():
    match = re.search(r"\bconstexpr\s+uintptr_t\s+" + re.escape(name) +
                      r"\s*=\s*(0x[0-9a-fA-F]+)", world)
    if not match or int(match.group(1), 16) != int(expected_value, 16):
        problems.append(f"LootField::{name}: expected {expected_value}")

for rva in ("0x64FA604", "0x8A06D60", "0x94DE944", "0x94DE990"):
    if rva not in hooks:
        problems.append(f"missing pickup hook RVA {rva}")

if sdk.count("HalfString *MarkOfName()") != 2:
    problems.append("name accessors must return HalfString*")
if "Class_WeaponConfConfig_LOCID_Name" not in sdk or "Class_LethalAndTacticalConfConfig_LOCID_Name" not in sdk:
    problems.append("missing separate weapon/throwable name field accessors")

if problems:
    print("\n".join(problems), file=sys.stderr)
    sys.exit(1)
print(f"PASS: {len(expected)} definitions, {len(fields)} pickup fields, 4 hook RVAs and name accessor types")

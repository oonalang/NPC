import argparse
import pathlib
import re
import sys

MAPPINGS = {
    "0x6D9BC4C": "SetUseItemTime",
    "0x6D9B670": "Tick",
    "0xC2B7A30": "get_AddHotTime",
    "0x54F50C4": "get_WeaponShake_Enable",
    "0x54F756C": "get_SlideTackleAcclerationSpeed",
    "0x7C696E8": "get_SlideTackleSpeed",
    "0x7C687C8": "GetSuperSlideRate",
}

def check_mapping(dump_path):
    expected = {int(rva, 16): name for rva, name in MAPPINGS.items()}
    found = {}
    candidate = None
    with dump_path.open("r", encoding="utf-8", errors="replace") as source:
        for line in source:
            match = re.search(r"RVA:\s*0x([0-9A-Fa-f]+)", line)
            if match:
                rva = int(match.group(1), 16)
                candidate = rva if rva in expected else None
            elif candidate is not None and re.search(r"\b" + re.escape(expected[candidate]) + r"\s*\(", line):
                found[candidate] = line.strip()
                candidate = None
    return expected, found

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dump", type=pathlib.Path)
    args = parser.parse_args()
    source = pathlib.Path(__file__).resolve().parents[1] / "src/main/jni/System/Hooks/Feats.h"
    features = source.read_text(encoding="utf-8")
    expected, found = check_mapping(args.dump)
    fails = []
    for rva, method in expected.items():
        literal = f'LICENSE_HOOK_LIB("libunity.so", "0x{rva:X}"'
        is_present = literal in features
        is_in_dump = rva in found
        status = "PASS" if is_present and is_in_dump else "FAIL"
        print(f"{status} {method} at 0x{rva:X}")
        if status == "FAIL":
            fails.append(method)
    for symbol in ("PlateTimeOff = 0x30", "UseCompTimeOff = 0xAC"):
        ok = symbol in features
        print(f"{'PASS' if ok else 'FAIL'} {symbol}")
        if not ok:
            fails.append(symbol)
    print("This checks source-to-dump declarations, not native instructions or device behavior.")
    return 1 if fails else 0

if __name__ == "__main__":
    sys.exit(main())

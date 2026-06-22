"""Download Mixamo combat slash clips (reuses mixamo_download's API helpers).

  MIXAMO_TOKEN=<jwt> python mixamo_download_combat.py

Grabs a few one-handed slash options so we can pick the best for the lightsaber attack.
in_place=True keeps the character rooted (no root-motion slide during the montage).
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mixamo_download as md

# (stem, Mixamo search query, in_place)
COMBAT = [
    ("Sword_Slash", "sword slash", True),
    ("Slash", "slash", True),
    ("Standing_Melee_Attack_Downward", "standing melee attack downward", True),
    ("Great_Sword_Slash", "great sword slash", True),
]


def main() -> int:
    token = os.environ.get("MIXAMO_TOKEN", "").strip()
    if not token:
        print("ERROR: set MIXAMO_TOKEN"); return 2
    os.makedirs(md.RAW_DIR, exist_ok=True)
    char = md.get_default_character(token)
    print("character_id=%s" % char)
    ok, fail = [], []
    for stem, query, inplace in COMBAT:
        dst = os.path.join(md.RAW_DIR, stem + ".fbx")
        if os.path.exists(dst) and os.path.getsize(dst) > 20_000:
            print("[skip] %s (exists)" % stem); ok.append(stem); continue
        try:
            md.export_and_download(token, char, stem, query, inplace, False)
            print("[ok]   %s" % stem); ok.append(stem)
        except Exception as e:  # noqa: BLE001
            print("[fail] %s: %s" % (stem, e)); fail.append("%s: %s" % (stem, e))
    print("\nSUMMARY ok=%d fail=%d" % (len(ok), len(fail)))
    for f in fail:
        print("  FAIL %s" % f)
    return 0 if not fail else 1


if __name__ == "__main__":
    sys.exit(main())

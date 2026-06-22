"""Fix the arena PBR materials so they actually render in-game.

The asset-gen pipeline created M_Arena_Floor/Wall/Pillar without the StaticLighting usage
flag, so on the statically-lit VerticalSlice the engine rejects them and falls back to the
default grey material ("missing usage flag StaticLighting") — that is the gray-box look.
Set the flag, recompile, and re-save so the real textured surfaces show.
"""
import unreal

MATS = [
    "/Game/ArenaBuild/M_Arena_Floor",
    "/Game/ArenaBuild/M_Arena_Wall",
    "/Game/ArenaBuild/M_Arena_Pillar",
]


def main():
    unreal.log("ARENA_MAT: BEGIN")
    eal = unreal.EditorAssetLibrary
    ok = True
    for p in MATS:
        m = unreal.load_asset(p)
        if not m:
            unreal.log_error("ARENA_MAT: MISSING %s" % p)
            ok = False
            continue
        try:
            m.set_editor_property("used_with_static_lighting", True)
        except Exception as e:
            unreal.log_warning("ARENA_MAT: %s static-lighting flag failed (%s)" % (p, e))
            ok = False
        unreal.MaterialEditingLibrary.recompile_material(m)
        eal.save_asset(p)
        unreal.log("ARENA_MAT: fixed %s" % p)
    unreal.log("[gate] RESULT=%s" % ("PASS" if ok else "FAIL"))


main()

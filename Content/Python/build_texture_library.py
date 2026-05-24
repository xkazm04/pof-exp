"""
build_texture_library.py
========================
Establish a stable, reusable surface-texture library at /Game/Textures/Surfaces/
(game §4) so future levels reuse the dungeon stone/brick/metal sets instead of
re-generating them per dispatch.

Non-destructive: DUPLICATES the arena's working textures into the library under
stable names (the arena's own bindings keep pointing at /Game/ArenaBuild/Textures).
Biome definitions ([[../05-environment]] §3) should point at these library
textures going forward. Idempotent -- skips entries that already exist.

Run headless:
    "<UE>/UnrealEditor.exe" "<...>/PoF.uproject" ^
        -ExecutePythonScript="<abs path>" -unattended -nopause -nosplash
"""
import unreal

asset_lib = unreal.EditorAssetLibrary

SRC = "/Game/ArenaBuild/Textures"
DST = "/Game/Textures/Surfaces"

# arena working texture -> stable library name
LIBRARY = {
    "T_floor_albedo":  "T_DungeonStone_01_Albedo",
    "T_floor_normal":  "T_DungeonStone_01_Normal",
    "T_floor_rough":   "T_DungeonStone_01_Roughness",
    "T_wall_albedo":   "T_DungeonBrick_01_Albedo",
    "T_wall_normal":   "T_DungeonBrick_01_Normal",
    "T_wall_rough":    "T_DungeonBrick_01_Roughness",
    "T_pillar_albedo": "T_DungeonMetal_01_Albedo",
    "T_pillar_normal": "T_DungeonMetal_01_Normal",
    "T_pillar_rough":  "T_DungeonMetal_01_Roughness",
}


def main():
    unreal.log("[build_texture_library] === START ===")
    if not asset_lib.does_directory_exist(DST):
        asset_lib.make_directory(DST)

    made, skipped, missing = 0, 0, 0
    for src_name, lib_name in LIBRARY.items():
        src = "%s/%s" % (SRC, src_name)
        dst = "%s/%s" % (DST, lib_name)
        if not asset_lib.does_asset_exist(src):
            unreal.log_warning("[build_texture_library] missing source " + src)
            missing += 1
            continue
        if asset_lib.does_asset_exist(dst):
            skipped += 1
            continue
        if asset_lib.duplicate_asset(src, dst) is not None:
            asset_lib.save_asset(dst)
            made += 1
            unreal.log("[build_texture_library] %s -> %s" % (src_name, lib_name))
        else:
            unreal.log_warning("[build_texture_library] duplicate failed for " + src)

    unreal.log("[build_texture_library] === DONE: %d new, %d existing, %d missing under %s ==="
               % (made, skipped, missing, DST))


if __name__ == "__main__":
    main()

"""
make_test_jedi_map.py — Stream 2 private map (deliverable).

Fresh map creation (new_level / new_blank_map) crashes in a -run=pythonscript
commandlet on 5.8, so build Test_Jedi by DUPLICATING the proven lit VerticalSlice
arena (asset-level copy, no world creation) and retargeting it to the Jedi:
  - duplicate VerticalSlice (+ _BuiltData) -> Test_Jedi
  - load_map (commandlet-safe), set WorldSettings game mode -> BP_JediGameMode,
    remove the enemy pawn, save_map.

Result: /Game/Maps/Test_Jedi spawns BP_JediPlayer in a lit arena with no
command-line override needed. Writes Saved/make_test_jedi_map.json.
"""
import json
import unreal

EAL = unreal.EditorAssetLibrary
SRC = "/Game/Maps/VerticalSlice"
DST = "/Game/Maps/Test_Jedi"
GM = "/Game/Characters/Jedi/BP_JediGameMode"
OUT = {"steps": []}


def _log(m):
    unreal.log_warning("[make_test_jedi_map] " + str(m))
    OUT["steps"].append(str(m))


def dup(src, dst):
    if EAL.does_asset_exist(dst):
        EAL.delete_asset(dst)
    ok = EAL.duplicate_asset(src, dst)
    _log("duplicate %s -> %s : %s" % (src, dst, "ok" if ok else "FAIL"))
    return ok


def main():
    _log("=== MAKE TEST_JEDI MAP START ===")
    if not EAL.does_asset_exist(SRC):
        _log("[gate] RESULT=FAIL (source map missing)")
        return
    dup(SRC, DST)
    # Baked-lighting data (keep the arena lit). May or may not relink; harmless if absent.
    if EAL.does_asset_exist(SRC + "_BuiltData"):
        dup(SRC + "_BuiltData", DST + "_BuiltData")

    world = unreal.EditorLoadingAndSavingUtils.load_map(DST)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    gm_bp = EAL.load_asset(GM)
    gm_set, removed = False, 0

    # WorldSettings is NOT returned by get_all_level_actors — fetch it explicitly.
    ws_list = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.WorldSettings)
    for ws in ws_list:
        ws.set_editor_property("default_game_mode", gm_bp.generated_class())
        gm_set = True
    _log("worldsettings found=%d" % len(ws_list))

    for a in list(eas.get_all_level_actors()):
        cn = a.get_class().get_name()
        if "Enemy" in cn:
            eas.destroy_actor(a)
            removed += 1
    _log("game mode set=%s, enemies removed=%d" % (gm_set, removed))

    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, DST)
    _log("save_map %s ok=%s" % (DST, saved))

    OUT["gamemode_set"] = gm_set
    OUT["enemies_removed"] = removed
    OUT["result"] = "PASS" if (gm_set and saved) else "FAIL"
    out_path = unreal.Paths.combine([unreal.Paths.project_saved_dir(), "make_test_jedi_map.json"])
    with open(out_path, "w", encoding="utf-8") as fh:
        json.dump(OUT, fh, indent=2)
    _log("[gate] RESULT=%s" % OUT["result"])


main()

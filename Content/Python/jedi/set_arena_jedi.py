"""Make Arena_Ancient spawn the Jedi: set its WorldSettings GameMode override to
BP_JediGameMode (DefaultPawnClass = BP_JediPlayer — robed Manny + lit saber).
Commandlet-safe (load_map/save_map). Writes Saved/set_arena_jedi.json."""
import json
import unreal

MAP = "/Game/Maps/Arena_Ancient"
GM = "/Game/Characters/Jedi/BP_JediGameMode"
OUT = {"steps": []}


def _log(m):
    unreal.log_warning("[set_arena_jedi] " + str(m))
    OUT["steps"].append(str(m))


def main():
    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    gm_bp = unreal.EditorAssetLibrary.load_asset(GM)
    if gm_bp is None:
        _log("[gate] RESULT=FAIL (BP_JediGameMode missing)")
        return
    n = 0
    for ws in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.WorldSettings):
        ws.set_editor_property("default_game_mode", gm_bp.generated_class())
        n += 1
    _log("set game mode -> BP_JediGameMode on %d WorldSettings" % n)
    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)
    _log("save_map ok=%s" % saved)
    OUT["result"] = "PASS" if (n > 0 and saved) else "FAIL"
    out_path = unreal.Paths.combine([unreal.Paths.project_saved_dir(), "set_arena_jedi.json"])
    with open(out_path, "w", encoding="utf-8") as fh:
        json.dump(OUT, fh, indent=2)
    _log("[gate] RESULT=%s" % OUT["result"])


main()

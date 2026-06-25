"""Make the Jedi the player in EVERY map: set BP_VSGameMode.DefaultPawnClass ->
BP_JediPlayer. BP_VSGameMode is the project's GlobalDefaultGameMode, so every map
that uses the default (or explicitly sets BP_VSGameMode) now spawns the Jedi.
Writes Saved/set_default_jedi_pawn.json."""
import json
import unreal

EAL = unreal.EditorAssetLibrary
VSGM = "/Game/VerticalSlice/BP_VSGameMode"
JEDI = "/Game/Characters/Jedi/BP_JediPlayer"
OUT = {"steps": []}


def _log(m):
    unreal.log_warning("[default_jedi] " + str(m))
    OUT["steps"].append(str(m))


def main():
    gm = EAL.load_asset(VSGM)
    jedi = EAL.load_asset(JEDI)
    if gm is None or jedi is None:
        _log("[gate] RESULT=FAIL (missing asset)")
        return
    jedi_class = jedi.generated_class()
    cdo = unreal.get_default_object(gm.generated_class())
    before = cdo.get_editor_property("default_pawn_class")
    _log("BP_VSGameMode default pawn BEFORE = %s" % (before.get_name() if before else None))
    cdo.set_editor_property("default_pawn_class", jedi_class)
    unreal.BlueprintEditorLibrary.compile_blueprint(gm)
    EAL.save_asset(VSGM)
    after = unreal.get_default_object(gm.generated_class()).get_editor_property("default_pawn_class")
    _log("BP_VSGameMode default pawn AFTER  = %s" % (after.get_name() if after else None))
    OUT["result"] = "PASS" if (after and "Jedi" in after.get_name()) else "FAIL"
    out_path = unreal.Paths.combine([unreal.Paths.project_saved_dir(), "set_default_jedi_pawn.json"])
    with open(out_path, "w", encoding="utf-8") as fh:
        json.dump(OUT, fh, indent=2)
    _log("[gate] RESULT=%s" % OUT["result"])


main()

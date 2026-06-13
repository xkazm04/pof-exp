"""Build /Game/Maps/GateArena — a clean, noise-free gating map for the verification gate.

Approach (chosen from PROBED ground truth, not assumption): VerticalSlice is NOT OFPA (actors are
embedded) and renders correctly thanks to a rich light rig (4 directional lights + 4 sky lights +
height fog + post-process). Building lighting from scratch rendered dark twice. So we DUPLICATE
VerticalSlice (inheriting its exact, known-good lighting + geometry + game mode + PIE setup) and
strip only the roaming actor that adds pixel noise to the golden-image diff: BP_VSEnemy_C (VSEnemy).
The open -Y lane the gate rolls down stays clear.

Call: /pof/python/run {module:"observation.make_gate_map", function:"run", args:{}}.
"""
import unreal

SRC = "/Game/Maps/VerticalSlice"


def run(args):
    dst = args.get("dst", "/Game/Maps/GateArena")
    remove_classes = set(args.get("remove_classes", ["BP_VSEnemy_C"]))
    out = {"src": SRC, "dst": dst, "removed": []}

    if unreal.EditorAssetLibrary.does_asset_exist(dst):
        unreal.EditorAssetLibrary.delete_asset(dst)
    dup = unreal.EditorAssetLibrary.duplicate_asset(SRC, dst)
    out["duplicated"] = dup is not None
    if not out["duplicated"]:
        return {**out, "error": "duplicate_asset failed (OFPA?)"}

    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.load_level(dst)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for a in list(eas.get_all_level_actors()):
        if a.get_class().get_name() in remove_classes:
            out["removed"].append(f"{a.get_class().get_name()} :: {a.get_actor_label()}")
            eas.destroy_actor(a)
    les.save_current_level()
    out["map"] = dst
    return out

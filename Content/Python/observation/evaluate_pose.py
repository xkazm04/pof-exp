"""EvaluatePose (SP1) — T3 behavioral truth.

mode=clip : sample the AnimSequence at two times, measure max bone movement.
            is_static==True => the clip is ref-pose/T-pose (no animation). This
            catches empty retargeted clips without PIE.
mode=pie  : read the live mesh component-space transforms in a running PIE world
            (a RunScenario must have started it).
"""
import unreal

from observation import make_observation

# Manny bones likely present post-retarget; the live API confirms names.
DEFAULT_BONES = ["pelvis", "spine_03", "hand_l", "hand_r", "foot_l", "foot_r"]
STATIC_THRESHOLD_CM = 0.5


def _bone_pos(anim, bone, time):
    t = unreal.AnimationLibrary.get_bone_pose_for_time(anim, bone, time, False)
    return (t.translation.x, t.translation.y, t.translation.z)


def _clip(args):
    anim = unreal.EditorAssetLibrary.load_asset(args["asset_path"])
    if not anim:
        return make_observation("pose", {"error": f"not found: {args['asset_path']}"})
    bones = args.get("bones") or DEFAULT_BONES
    length = anim.get_play_length()
    t0, t1 = 0.0, max(length * 0.5, 0.01)
    max_delta = 0.0
    for b in bones:
        try:
            p0, p1 = _bone_pos(anim, b, t0), _bone_pos(anim, b, t1)
            max_delta = max(max_delta, max(abs(p0[i] - p1[i]) for i in range(3)))
        except Exception:
            continue
    return make_observation("pose", {
        "asset_path": args["asset_path"],
        "length": length,
        "max_bone_delta_over_time": max_delta,
        "is_static": max_delta < STATIC_THRESHOLD_CM,
        "bones_sampled": bones,
    })


def _pie(args):
    world = None
    if hasattr(unreal, "UnrealEditorSubsystem"):
        world = unreal.UnrealEditorSubsystem().get_game_world()
    if not world:
        return make_observation("pose", {"error": "no PIE world (run a Scenario first)"})
    pawn = unreal.GameplayStatics.get_player_pawn(world, 0)
    if not pawn:
        return make_observation("pose", {"error": "no player pawn in PIE"})
    mesh = pawn.get_component_by_class(unreal.SkeletalMeshComponent)
    xforms = []
    if mesh and hasattr(mesh, "get_component_space_transforms"):
        xforms = mesh.get_component_space_transforms()
    return make_observation("pose", {
        "bone_count": len(xforms),
        "has_mesh": bool(mesh),
    }, scenario_id=args.get("scenario_id"))


def run(args):
    mode = args.get("mode", "clip")
    if mode == "clip":
        return _clip(args)
    if mode == "pie":
        return _pie(args)
    return make_observation("pose", {"error": f"unknown mode: {mode}"})

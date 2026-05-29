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
# Limb bones that swing most during locomotion. Rotation (not translation) is the
# correct "is this clip animated?" signal — local bone translations are ~constant
# for any clip (bone lengths are fixed); only rotations carry the animation.
DEFAULT_BONES = ["thigh_l", "thigh_r", "calf_l", "calf_r", "upperarm_l", "lowerarm_l", "foot_l", "foot_r"]
STATIC_THRESHOLD_DEG = 3.0


def _bone_rot(anim, bone, time):
    """Return the bone's local rotation quaternion (x, y, z, w) at a time."""
    t = unreal.AnimationLibrary.get_bone_pose_for_time(anim, bone, time, False)
    q = t.rotation
    return (q.x, q.y, q.z, q.w)


def _quat_angle_deg(q0, q1):
    import math
    dot = abs(sum(q0[i] * q1[i] for i in range(4)))
    dot = min(1.0, max(-1.0, dot))
    return math.degrees(2.0 * math.acos(dot))


def _clip(args):
    anim = unreal.EditorAssetLibrary.load_asset(args["asset_path"])
    if not anim:
        return make_observation("pose", {"error": f"not found: {args['asset_path']}"})
    bones = args.get("bones") or DEFAULT_BONES
    length = anim.get_play_length()
    # Sample several phases; max rotation delta from frame 0 across the cycle.
    times = [length * f for f in (0.0, 0.25, 0.5, 0.75)]
    base = {}
    max_rot_deg = 0.0
    sampled = []
    for b in bones:
        try:
            q0 = _bone_rot(anim, b, times[0])
        except Exception:
            continue
        sampled.append(b)
        for t in times[1:]:
            try:
                d = _quat_angle_deg(q0, _bone_rot(anim, b, t))
                max_rot_deg = max(max_rot_deg, d)
            except Exception:
                continue
    return make_observation("pose", {
        "asset_path": args["asset_path"],
        "length": length,
        "max_bone_rotation_deg": round(max_rot_deg, 2),
        "is_static": max_rot_deg < STATIC_THRESHOLD_DEG,
        "bones_sampled": sampled,
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

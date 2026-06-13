"""OFFLINE pose analysis — read an animation's actual bone WORLD positions per frame (no PIE,
no lighting, deterministic) via the modern AnimPoseExtensions API, and report the lowest point
per frame. Trustworthy floor-clip metric (vs the conservative mesh-bounds box).

Call: /pof/python/run {module:"observation.pose_floor", function:"run",
args:{asset:"/Game/Mixamo/Retargeted/SKM_Manny/Forward_Roll_RT"}}.
"""
import unreal


def run(args):
    path = args.get("asset", "/Game/Mixamo/Retargeted/SKM_Manny/Forward_Roll_RT")
    anim = unreal.load_asset(path)
    if not anim:
        return {"error": "not found", "path": path}
    out = {}
    APE = getattr(unreal, "AnimPoseExtensions", None)
    if APE is None:
        return {"error": "AnimPoseExtensions not exposed"}
    try:
        opts = unreal.AnimPoseEvaluationOptions()
    except Exception as e:
        return {"error": f"AnimPoseEvaluationOptions: {e}"}
    try:
        n = unreal.AnimationLibrary.get_num_frames(anim)
        out["frames"] = n
    except Exception as e:
        return {"error": f"get_num_frames: {e}"}

    try:
        pose0 = APE.get_anim_pose_at_frame(anim, 0, opts)
    except Exception as e:
        return {**out, "error": f"get_anim_pose_at_frame: {e}"}
    try:
        bnames = [str(x) for x in APE.get_bone_names(pose0)]
        out["num_bones"] = len(bnames)
    except Exception as e:
        return {**out, "error": f"get_bone_names: {e}"}

    space = unreal.AnimPoseSpaces.WORLD
    # Curated real body bones that can visibly contact/clip the floor (no IK/virtual/twist noise).
    WHITE = ("foot_l", "foot_r", "ball_l", "ball_r", "calf_l", "calf_r", "thigh_l", "thigh_r",
             "pelvis", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05", "neck_01",
             "head", "clavicle_l", "clavicle_r", "upperarm_l", "upperarm_r", "lowerarm_l",
             "lowerarm_r", "hand_l", "hand_r")
    real = [b for b in WHITE if b in bnames]
    out["tracked_bones"] = real

    def lowest(pose):
        lo, lob = 1e9, ""
        for nm in real:
            try:
                z = APE.get_bone_pose(pose, unreal.Name(nm), space).translation.z
            except Exception:
                continue
            if z < lo:
                lo, lob = z, nm
        return lo, lob

    # Floor reference = lowest FOOT/BALL bone over the whole clip (the true contact height).
    feet = [b for b in ("foot_l", "foot_r", "ball_l", "ball_r") if b in bnames]
    floor_ref = 1e9
    minz_per_frame = []
    lowbone_per_frame = []
    for f in range(n + 1):
        try:
            pose = APE.get_anim_pose_at_frame(anim, f, opts)
        except Exception:
            break
        lo, lob = lowest(pose)
        minz_per_frame.append(lo)
        lowbone_per_frame.append(lob)
        for fb in feet:
            try:
                fz = APE.get_bone_pose(pose, unreal.Name(fb), space).translation.z
                if fz < floor_ref:
                    floor_ref = fz
            except Exception:
                pass
    out["floor_ref_z"] = round(floor_ref, 1)

    clip_curve = [round(floor_ref - z, 1) for z in minz_per_frame]
    worst = max(range(len(clip_curve)), key=lambda i: clip_curve[i]) if clip_curve else -1
    out["max_clip_below_floor"] = clip_curve[worst] if worst >= 0 else None
    out["worst_frame"] = worst
    out["worst_bone"] = lowbone_per_frame[worst] if worst >= 0 else None
    # show which bone is lowest at each sampled frame
    step = max(1, len(clip_curve) // 12)
    out["samples"] = [{"f": i, "clip": clip_curve[i], "bone": lowbone_per_frame[i]} for i in range(0, len(clip_curve), step)]
    return out

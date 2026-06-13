"""Know-before-act PROBES — query engine GROUND TRUTH so I never act on an assumption.

Every probe answers a question I would otherwise guess. The two that would have caught this
project's worst bugs:
  - root_motion(asset): does this clip ACTUALLY travel, how far, and along WHICH bone axis?
       -> catches "the retarget dropped root motion" (magnitude ~= 0) AND
          "I assumed +X is forward" (reports the real dominant axis to bake along).
  - mesh_orientation(class): the character mesh's yaw offset, i.e. how bone-space maps to ACTOR
       space, so a bake/aim done in the right frame.

Bridge module (runs in-editor). Call:
  /pof/python/run {module:"observation.probe", function:"root_motion", args:{asset:"/Game/..."}}
"""
import math

import unreal


def root_motion(args):
    path = args["asset"]
    anim = unreal.load_asset(path)
    if not anim or not isinstance(anim, unreal.AnimSequence):
        return {"error": "not an AnimSequence", "asset": path}
    out = {"asset": path}
    try:
        out["enable_root_motion"] = bool(anim.get_editor_property("enable_root_motion"))
    except Exception as e:
        out["enable_root_motion_err"] = str(e)
    try:
        n = unreal.AnimationLibrary.get_num_frames(anim)
        out["frames"] = n
        out["length"] = round(float(anim.get_play_length()), 3)
    except Exception as e:
        return {**out, "error": f"frame count: {e}"}

    APE = getattr(unreal, "AnimPoseExtensions", None)
    if APE is None:
        return {**out, "error": "AnimPoseExtensions not exposed"}
    opts = unreal.AnimPoseEvaluationOptions()
    space = unreal.AnimPoseSpaces.WORLD  # top-level root bone in world == its root-motion trajectory (no actor)

    bone = args.get("root_bone", "root")

    def root_at(f):
        pose = APE.get_anim_pose_at_frame(anim, f, opts)
        t = APE.get_bone_pose(pose, unreal.Name(bone), space).translation
        return (t.x, t.y, t.z)

    try:
        p0, pN = root_at(0), root_at(n)
    except Exception as e:
        return {**out, "error": f"root pose read: {e}"}
    dx, dy, dz = pN[0] - p0[0], pN[1] - p0[1], pN[2] - p0[2]
    mag = math.hypot(dx, dy)
    out["root_delta_component"] = {"x": round(dx, 1), "y": round(dy, 1), "z": round(dz, 1)}
    out["horizontal_magnitude"] = round(mag, 1)
    if abs(dx) >= abs(dy):
        out["dominant_bone_axis"] = "+x" if dx > 0 else "-x"
    else:
        out["dominant_bone_axis"] = "+y" if dy > 0 else "-y"
    out["travels"] = mag > 20.0
    out["verdict"] = (
        "DROPPED — no usable root motion (retarget likely stripped it); will not move without a bake"
        if not out["travels"] else
        f"travels {mag:.0f}u along bone {out['dominant_bone_axis']} — bake/expect motion on THIS axis"
    )
    return out


def level_actors(args):
    """List a level's actors (label+class) and whether it uses external actors (OFPA) — so a
    'duplicate + strip enemy' plan is built on fact, not assumption."""
    m = args["map"]
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.load_level(m)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = eas.get_all_level_actors()
    out = {"map": m, "count": len(actors)}
    # Flag likely enemies/AI/pawns + obstacles so the strip target is explicit.
    out["actors"] = sorted({f"{a.get_class().get_name()}  ::  {a.get_actor_label()}" for a in actors})
    try:
        w = unreal.EditorLevelLibrary.get_editor_world()
        lvl = w.get_current_level()
        out["uses_external_actors"] = bool(lvl.get_editor_property("use_external_actors"))
    except Exception as e:
        out["ofpa_probe_err"] = str(e)
    return out


def mesh_orientation(args):
    """Report the character mesh component's yaw offset, so bone-space -> actor-space is explicit."""
    bp = args.get("class", "/Game/VerticalSlice/BP_VSPlayer.BP_VSPlayer_C")
    out = {"class": bp}
    try:
        cls = unreal.load_object(None, bp)
        cdo = unreal.get_default_object(cls)
        mesh = cdo.get_editor_property("mesh")
        rot = mesh.get_editor_property("relative_rotation")
        out["mesh_relative_yaw"] = round(rot.yaw, 1)
        out["note"] = ("bake/aim in ACTOR space; a non-zero mesh yaw means bone-space +X is NOT "
                       "actor +X. Standard UE character mesh yaw is -90 (bone +Y -> actor +X).")
    except Exception as e:
        out["error"] = str(e)
    return out

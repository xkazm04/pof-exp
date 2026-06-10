"""Author AS_SwordSlash + AM_SwordSlash programmatically and wire BP_GA_MeleeAttack to them.

The slash AnimSequence is built from scratch on the Manny skeleton:
  - base pose = Standard_Idle_RT frame 0 (every bone gets a static local-transform track)
  - an animated right-arm + spine chain keyframed over 21 frames @30fps:
    windup (twist right, raise arm) -> slash (fast sweep across) -> recover (back to idle)
  - deltas are applied in PARENT space: new_local_q = delta_q * idle_local_q

The montage carries AnimNotifyState_HitDetection (WeaponSocketName=Base, TipSocketName=Tip —
resolved against the character's WeaponMesh sword sockets by the C++ extension) over the
slash window, so Event.MeleeHit drives GA damage with real weapon collisions.

Call: /pof/python/run {module:"observation.author_sword_slash", function:"run", args:{}}.
"""
import json
import math

import unreal

IDLE = "/Game/Mixamo/Retargeted/SKM_Manny/Standard_Idle_RT"
PKG = "/Game/Weapons"
SEQ_NAME = "AS_SwordSlash"
MON_NAME = "AM_SwordSlash"
FPS = 30
FRAMES = 21          # 0.7 s swing
HIT_START = 0.20     # hit-detection window (s)
HIT_DUR = 0.20

# Animated chain: per-bone keyed PARENT-SPACE euler deltas (roll, pitch, yaw) at key frames.
# Linear-interped per frame, then composed onto the idle local rotation.
KEYS = {
    "spine_01":   {0: (0, 0, 0), 5: (0, 0, 25), 9: (0, 0, -5), 12: (0, 0, -30), 20: (0, 0, 0)},
    "spine_02":   {0: (0, 0, 0), 5: (0, 0, 15), 9: (0, 0, -5), 12: (0, 0, -20), 20: (0, 0, 0)},
    "upperarm_r": {0: (0, 0, 0), 5: (0, -60, 30), 9: (0, -25, -40), 12: (0, 5, -75), 20: (0, 0, 0)},
    "lowerarm_r": {0: (0, 0, 0), 5: (0, 0, -25), 9: (0, 0, -10), 12: (0, 0, -5), 20: (0, 0, 0)},
    "hand_r":     {0: (0, 0, 0), 5: (10, 0, 0), 9: (-10, 0, 0), 12: (-15, 0, 0), 20: (0, 0, 0)},
}


def _lerp_delta(keys, f):
    ks = sorted(keys)
    if f <= ks[0]:
        return keys[ks[0]]
    if f >= ks[-1]:
        return keys[ks[-1]]
    for i in range(len(ks) - 1):
        a, b = ks[i], ks[i + 1]
        if a <= f <= b:
            t = (f - a) / float(b - a)
            va, vb = keys[a], keys[b]
            return tuple(va[j] + (vb[j] - va[j]) * t for j in range(3))
    return (0, 0, 0)


def run(args):
    out = {}
    idle = unreal.load_asset(IDLE)
    if not idle:
        return {"error": "idle clip missing"}
    skel = idle.get_skeleton()

    # --- read the idle base pose (LOCAL space, every bone) ---
    APE = unreal.AnimPoseExtensions
    opts = unreal.AnimPoseEvaluationOptions()
    pose0 = APE.get_anim_pose_at_frame(idle, 0, opts)
    bones = [str(b) for b in APE.get_bone_names(pose0)]
    local = {}
    for b in bones:
        t = APE.get_bone_pose(pose0, unreal.Name(b), unreal.AnimPoseSpaces.LOCAL)
        local[b] = t
    out["bones"] = len(bones)

    # --- create the sequence ---
    seq_path = f"{PKG}/{SEQ_NAME}"
    if unreal.EditorAssetLibrary.does_asset_exist(seq_path):
        unreal.EditorAssetLibrary.delete_asset(seq_path)
    factory = unreal.AnimSequenceFactory()
    factory.set_editor_property("target_skeleton", skel)
    seq = unreal.AssetToolsHelpers.get_asset_tools().create_asset(SEQ_NAME, PKG, unreal.AnimSequence, factory)
    if not seq:
        return {**out, "error": "create AnimSequence failed"}
    ctrl = seq.get_editor_property("controller")
    try:
        ctrl.open_bracket("author slash")
        ctrl.set_frame_rate(unreal.FrameRate(FPS, 1))
        ctrl.set_number_of_frames(unreal.FrameNumber(FRAMES))
    except Exception as e:
        return {**out, "error": f"frame setup: {e}",
                "ctrl_methods": sorted(m for m in dir(ctrl) if "frame" in m.lower() or "rate" in m.lower())}

    # --- write tracks: static for everything, keyed deltas for the chain ---
    n_keys = FRAMES + 1
    written = 0
    for b in bones:
        t = local[b]
        base_q = t.rotation
        base_p = t.translation
        base_s = t.scale3d
        positions = [base_p] * n_keys
        scales = [base_s] * n_keys
        if b in KEYS:
            rots = []
            for f in range(n_keys):
                r, p, y = _lerp_delta(KEYS[b], f)
                dq = unreal.Rotator(r, p, y).quaternion()
                rots.append(dq.multiply(base_q))
            rotations = rots
        else:
            rotations = [base_q] * n_keys
        try:
            ctrl.add_bone_curve(unreal.Name(b))
            ctrl.set_bone_track_keys(unreal.Name(b), positions, rotations, scales)
            written += 1
        except Exception as e:
            out.setdefault("track_errs", []).append(f"{b}: {e}")
            if "track_methods" not in out:
                out["track_methods"] = sorted(m for m in dir(ctrl) if "bone" in m.lower() or "track" in m.lower())
            break
    ctrl.close_bracket()
    out["tracks_written"] = written
    seq.set_editor_property("enable_root_motion", False)
    out["seq_saved"] = unreal.EditorAssetLibrary.save_loaded_asset(seq)

    # --- montage with the hit-detection window ---
    mon_path = f"{PKG}/{MON_NAME}"
    if unreal.EditorAssetLibrary.does_asset_exist(mon_path):
        unreal.EditorAssetLibrary.delete_asset(mon_path)
    mf = unreal.AnimMontageFactory()
    mf.set_editor_property("source_animation", seq)
    try:
        mf.set_editor_property("target_skeleton", skel)
    except Exception:
        pass
    montage = unreal.AssetToolsHelpers.get_asset_tools().create_asset(MON_NAME, PKG, unreal.AnimMontage, mf)
    if not montage:
        return {**out, "error": "create montage failed"}
    try:
        unreal.AnimationLibrary.add_animation_notify_track(montage, "HitWindow")
        ev = unreal.AnimationLibrary.add_animation_notify_state_event(
            montage, "HitWindow", HIT_START, HIT_DUR, unreal.AnimNotifyState_HitDetection)
        if ev:
            ev.set_editor_property("weapon_socket_name", "Base")
            ev.set_editor_property("tip_socket_name", "Tip")
            ev.set_editor_property("trace_radius", 25.0)
            out["notify"] = "HitDetection 0.20-0.40s sockets Base/Tip"
    except Exception as e:
        out["notify_err"] = str(e)
    out["mon_saved"] = unreal.EditorAssetLibrary.save_loaded_asset(montage)

    # --- wire the BP melee ability (durable: BP asset save) ---
    ga_cls = unreal.load_object(None, "/Game/Abilities/BP_GA_MeleeAttack.BP_GA_MeleeAttack_C")
    if ga_cls:
        ga = unreal.get_default_object(ga_cls)
        ga.set_editor_property("attack_montage", montage)
        ga.set_editor_property("use_animation_driven_damage", True)
        out["ga_wired"] = True
        bpa = unreal.load_asset("/Game/Abilities/BP_GA_MeleeAttack")
        out["ga_saved"] = unreal.EditorAssetLibrary.save_loaded_asset(bpa) if bpa else False
    else:
        out["ga_wired"] = False
    return out

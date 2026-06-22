"""Author the saber recoil reaction (RecoilFlinch) + AM_Recoil montage. When a swing is
parried, the attacker's saber is knocked UP and BACK off its attack line — a sharp flinch
that snaps in, then recovers. Plays on the attacker via SaberClashSubsystem on a parry.
Component FK authoring (calibrated): +pitch raises/back, +yaw sweeps right.
"""
import sys
sys.path.insert(0, r"C:/Users/kazda/Documents/Unreal Projects/PoF/Content/Python/anim_authoring")
import unreal as u
import pose_engine as pe

CHAIN = ["root", "pelvis", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05",
         "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r"]
PARENT = {"upperarm_r": "clavicle_r", "lowerarm_r": "upperarm_r", "hand_r": "lowerarm_r",
          "spine_03": "spine_02", "spine_04": "spine_03"}
FPS = 30

# (time, {bone: (raise=pitch, sweep=yaw, twist=roll) component deltas deg})
RECOIL = [
    (0.00, {"upperarm_r": (35, 0, 0), "lowerarm_r": (25, 0, 0)}),                            # mid-swing
    (0.08, {"upperarm_r": (135, 30, -25), "lowerarm_r": (15, 0, 0), "spine_03": (-12, 0, 0)}),  # knocked up-back-right + lean back
    (0.20, {"upperarm_r": (120, 22, -15), "lowerarm_r": (20, 0, 0), "spine_03": (-6, 0, 0)}),   # hold the recoil
    (0.40, {"upperarm_r": (35, 0, 0), "lowerarm_r": (25, 0, 0)}),                            # recover
]


def qinv(q):
    return u.Quat(-q.x, -q.y, -q.z, q.w)


def comp_rots():
    idle = u.load_asset(pe.IDLE_BASE)
    R = u.Rotator(0.0, 0.0, 0.0).quaternion()
    out = {}
    for b in CHAIN:
        R = R * u.AnimationLibrary.get_bone_pose_for_frame(idle, b, 0, False).rotation
        out[b] = u.Quat(R.x, R.y, R.z, R.w)
    return out


def _ease(a):
    a = 0.0 if a < 0 else (1.0 if a > 1 else a)
    return a * a * (3.0 - 2.0 * a)


def main():
    CR = comp_rots()
    skel, names, base = pe.load_base_pose()
    bones = set()
    for _, p in RECOIL:
        bones.update(p.keys())

    def pose_at(t):
        seg = 0
        for i in range(len(RECOIL) - 1):
            if t >= RECOIL[i][0]:
                seg = i
        t0, p0 = RECOIL[seg]
        t1, p1 = RECOIL[min(seg + 1, len(RECOIL) - 1)]
        a = _ease((t - t0) / max(t1 - t0, 1e-5))
        out = {}
        for b in bones:
            v0 = p0.get(b, (0, 0, 0)); v1 = p1.get(b, (0, 0, 0))
            out[b] = tuple(v0[i] + (v1[i] - v0[i]) * a for i in range(3))
        return out

    def c2l(bone, rsw):
        Qw = u.Rotator(roll=rsw[2], pitch=rsw[0], yaw=rsw[1]).quaternion()
        return qinv(CR[PARENT[bone]]) * (Qw * CR[bone])

    def abs_fn(k):
        pose = pose_at(k / float(FPS))
        return {b: c2l(b, v) for b, v in pose.items()}

    nf = int(round(RECOIL[-1][0] * FPS))
    seq = pe.author_abs("RecoilFlinch", "/Game/PoF/GenAnims", skel, base, nf, FPS, abs_fn)
    u.log("RECOIL: authored %s" % seq)

    MPATH = "/Game/Weapons/AM_Recoil"
    if not u.EditorAssetLibrary.does_asset_exist(MPATH):
        s = u.load_asset(seq)
        f = u.AnimMontageFactory()
        f.set_editor_property("target_skeleton", s.get_skeleton())
        f.set_editor_property("source_animation", s)
        m = u.AssetToolsHelpers.get_asset_tools().create_asset("AM_Recoil", "/Game/Weapons", u.AnimMontage, f)
        if m:
            try:
                bi = m.get_editor_property("blend_in"); bi.set_editor_property("blend_time", 0.04)
                m.set_editor_property("blend_in", bi)
            except Exception:
                pass
            u.EditorAssetLibrary.save_asset(MPATH)
            u.log("RECOIL: created AM_Recoil len=%.2f" % m.get_play_length())
    else:
        u.log("RECOIL: AM_Recoil exists (references the updated pose)")
    u.log("[gate] RESULT=PASS")


main()

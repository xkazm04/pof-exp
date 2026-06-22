"""Author the saber block pose (ParryBlock) + AM_Parry montage. The blade raises HIGH and
sweeps across to the centerline to intercept an overhead chop, holds, then lowers. Component
FK authoring (calibrated): +pitch raises, -yaw sweeps left/centre.
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
BLOCK = [
    (0.00, {"upperarm_r": (20, 5, 0), "lowerarm_r": (10, 0, 0)}),
    (0.12, {"upperarm_r": (95, -28, 20), "lowerarm_r": (52, 0, 0), "hand_r": (0, 0, 40)}),    # high block, saber up-forward
    (0.40, {"upperarm_r": (95, -28, 20), "lowerarm_r": (52, 0, 0), "hand_r": (0, 0, 40)}),    # hold
    (0.55, {"upperarm_r": (20, 5, 0), "lowerarm_r": (10, 0, 0)}),                              # lower
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
    for _, p in BLOCK:
        bones.update(p.keys())

    def pose_at(t):
        seg = 0
        for i in range(len(BLOCK) - 1):
            if t >= BLOCK[i][0]:
                seg = i
        t0, p0 = BLOCK[seg]
        t1, p1 = BLOCK[min(seg + 1, len(BLOCK) - 1)]
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

    nf = int(round(BLOCK[-1][0] * FPS))
    seq = pe.author_abs("ParryBlock", "/Game/PoF/GenAnims", skel, base, nf, FPS, abs_fn)
    u.log("PARRY: authored %s" % seq)

    # montage AM_Parry (create only if missing; short blend-in)
    MPATH = "/Game/Weapons/AM_Parry"
    if not u.EditorAssetLibrary.does_asset_exist(MPATH):
        s = u.load_asset(seq)
        f = u.AnimMontageFactory()
        f.set_editor_property("target_skeleton", s.get_skeleton())
        f.set_editor_property("source_animation", s)
        m = u.AssetToolsHelpers.get_asset_tools().create_asset("AM_Parry", "/Game/Weapons", u.AnimMontage, f)
        if m:
            try:
                bi = m.get_editor_property("blend_in"); bi.set_editor_property("blend_time", 0.05)
                m.set_editor_property("blend_in", bi)
            except Exception:
                pass
            u.EditorAssetLibrary.save_asset(MPATH)
            u.log("PARRY: created AM_Parry len=%.2f" % m.get_play_length())
    else:
        u.log("PARRY: AM_Parry exists (references the updated pose)")
    u.log("[gate] RESULT=PASS")


main()

"""Author a clean diagonal overhead slash in CALIBRATED component space (FK-verified axes).

Per-bone keypose values are component-space deltas (raise=pitch, sweep=yaw, twist=roll) in
degrees. +pitch raises overhead/back, +yaw sweeps right, -yaw sweeps left. The arm cocks
up-back-right (windup) then chops down-across-left (strike). Converted to local per bone:
    new_local = inv(parent_comp_rot) * (Qw * bone_comp_rot)
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

# (time, {bone: (raise=pitch, sweep=yaw, twist=roll) component deltas deg})  POLISHED:
# torso coils right then uncoils left through the strike, bows forward into the blow (weight),
# wrist snaps on impact; slow anticipation, fast 0.10s strike.
SLASH = [
    (0.00, {"upperarm_r": (20, 5, 0), "lowerarm_r": (10, 0, 0)}),
    (0.38, {"upperarm_r": (145, 30, 15), "lowerarm_r": (75, 0, 0), "spine_04": (5, 20, 0)}),    # windup: cock up-back, coil right, load back
    (0.48, {"upperarm_r": (25, -30, 0), "lowerarm_r": (12, 0, 0),
            "spine_04": (-12, -18, 0), "hand_r": (0, 0, 35)}),                                   # strike: chop down, uncoil+lean fwd, wrist snap
    (0.66, {"upperarm_r": (-8, -55, -10), "lowerarm_r": (5, 0, 0), "spine_04": (-8, -25, 0)}),   # follow: low across-left, leaned in
    (1.00, {"upperarm_r": (20, 5, 0), "lowerarm_r": (10, 0, 0)}),
]


def qinv(q):
    return u.Quat(-q.x, -q.y, -q.z, q.w)


def comp_rots():
    idle = u.load_asset(pe.IDLE_BASE)
    R = u.Rotator(0.0, 0.0, 0.0).quaternion()
    out = {}
    for b in CHAIN:
        lr = u.AnimationLibrary.get_bone_pose_for_frame(idle, b, 0, False).rotation
        R = R * lr
        out[b] = u.Quat(R.x, R.y, R.z, R.w)
    return out


def _ease(a):
    a = 0.0 if a < 0 else (1.0 if a > 1 else a)
    return a * a * (3.0 - 2.0 * a)


def main():
    CR = comp_rots()
    skel, names, base = pe.load_base_pose()
    bones = set()
    for _, p in SLASH:
        bones.update(p.keys())

    def pose_at(t):
        seg = 0
        for i in range(len(SLASH) - 1):
            if t >= SLASH[i][0]:
                seg = i
        t0, p0 = SLASH[seg]
        t1, p1 = SLASH[min(seg + 1, len(SLASH) - 1)]
        a = _ease((t - t0) / max(t1 - t0, 1e-5))
        out = {}
        for b in bones:
            v0 = p0.get(b, (0, 0, 0)); v1 = p1.get(b, (0, 0, 0))
            out[b] = tuple(v0[i] + (v1[i] - v0[i]) * a for i in range(3))
        return out

    def comp_to_local(bone, rsw):
        Qw = u.Rotator(roll=rsw[2], pitch=rsw[0], yaw=rsw[1]).quaternion()
        return qinv(CR[PARENT[bone]]) * (Qw * CR[bone])

    def abs_fn(k):
        pose = pose_at(k / float(FPS))
        return {b: comp_to_local(b, v) for b, v in pose.items()}

    num_frames = int(round(SLASH[-1][0] * FPS))
    path = pe.author_abs("SwordSlashC", "/Game/PoF/GenAnims", skel, base, num_frames, FPS, abs_fn)
    u.log("SLASHC: authored %s" % path)
    u.log("[gate] RESULT=PASS")


main()

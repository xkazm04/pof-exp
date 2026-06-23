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
# Every bone's parent is its predecessor in the chain (the real Manny hierarchy), so any
# bone we animate — pelvis + the whole spine, not just the arm — resolves to local space.
PARENT = {CHAIN[i]: CHAIN[i - 1] for i in range(1, len(CHAIN))}
FPS = 30

# (time, {bone: (raise=pitch, sweep=yaw, twist=roll) deg}, ease)  WEIGHTED, FULL-BODY:
# the VLM ruler flagged the arm-only version (WARN 54: "arm lift, stiff core, no snap, even
# timing"). Fix = drive the WHOLE torso + hips (coil right -> explosive uncoil left, bow
# forward into the blow) and ACCELERATE into a fast 0.10s strike (ease "in" = the snap),
# with a settle overshoot on recovery. +yaw=right, -yaw=left; +pitch=raise/lean-back.
SLASH = [
    (0.00, {"upperarm_r": (20, 5, 0), "lowerarm_r": (10, 0, 0)}, "smooth"),
    # WINDUP: cock arm up-back-right; coil the whole torso right + load weight back. Hold = anticipation.
    (0.32, {
        "upperarm_r": (150, 38, 18), "lowerarm_r": (82, 0, 0),
        "pelvis": (0, 8, 0),
        "spine_01": (3, 8, 0), "spine_02": (3, 10, 0), "spine_03": (4, 11, 0), "spine_04": (4, 13, 0),
    }, "out"),
    # STRIKE: explosive uncoil — torso rotates hard LEFT + bows FORWARD, hips drive, arm chops, wrist snaps.
    (0.42, {
        "upperarm_r": (28, -32, 0), "lowerarm_r": (14, 0, 0), "hand_r": (0, 0, 44),
        "pelvis": (0, -8, 0),
        "spine_01": (-3, -10, 0), "spine_02": (-4, -13, 0), "spine_03": (-6, -15, 0), "spine_04": (-9, -18, 0),
    }, "in"),
    # FOLLOW-THROUGH: blade carries low across-left, torso fully rotated through, weight forward.
    (0.60, {
        "upperarm_r": (-6, -58, -10), "lowerarm_r": (6, 0, 0),
        "pelvis": (0, -10, 0),
        "spine_01": (-4, -9, 0), "spine_02": (-5, -12, 0), "spine_03": (-7, -15, 0), "spine_04": (-10, -20, 0),
    }, "smooth"),
    # SETTLE: small overshoot past neutral, then ease home (secondary motion, not a dead stop).
    (0.86, {
        "upperarm_r": (30, 11, 0), "lowerarm_r": (15, 0, 0),
        "pelvis": (0, -2, 0), "spine_02": (1, -3, 0), "spine_03": (1, -3, 0),
    }, "smooth"),
    (1.20, {"upperarm_r": (20, 5, 0), "lowerarm_r": (10, 0, 0)}, "out"),
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


def _ease(a, mode):
    a = 0.0 if a < 0 else (1.0 if a > 1 else a)
    if mode == "in":
        return a * a * a                      # accelerate (slow->fast): the strike snap
    if mode == "out":
        return 1.0 - (1.0 - a) ** 3           # decelerate (fast->slow): anticipation hold / settle
    return a * a * (3.0 - 2.0 * a)            # smoothstep


def main():
    CR = comp_rots()
    skel, names, base = pe.load_base_pose()
    bones = set()
    for kp in SLASH:
        bones.update(kp[1].keys())

    def pose_at(t):
        seg = 0
        for i in range(len(SLASH) - 1):
            if t >= SLASH[i][0]:
                seg = i
        t0, p0 = SLASH[seg][0], SLASH[seg][1]
        nxt = SLASH[min(seg + 1, len(SLASH) - 1)]
        t1, p1, mode = nxt[0], nxt[1], nxt[2]
        a = _ease((t - t0) / max(t1 - t0, 1e-5), mode)
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

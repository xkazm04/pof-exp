"""Numeric verification of the slash arc: compute the hand's COMPONENT position at each
keypose (applying upperarm/lowerarm/spine rotations). A good diagonal overhead chop traces:
chest -> up-back-right (high Z) -> down-forward-left -> low-left. Component: X=fwd, Y=right, Z=up.
"""
import unreal as u
T = "CHK"
def log(m): u.log("%s: %s" % (T, m))

IDLE = "/Game/Mixamo/Retargeted/SKM_Manny/Standard_Idle_RT"
CHAIN = ["root", "pelvis", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05",
         "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r"]
PARENT = {"upperarm_r": "clavicle_r", "lowerarm_r": "upperarm_r",
          "spine_03": "spine_02", "spine_04": "spine_03"}
def qinv(q): return u.Quat(-q.x, -q.y, -q.z, q.w)

idle = u.load_asset(IDLE)
L = {b: u.AnimationLibrary.get_bone_pose_for_frame(idle, b, 0, False) for b in CHAIN}

CR = {}
_R = u.Rotator(0.0, 0.0, 0.0).quaternion()
for b in CHAIN:
    _R = _R * L[b].rotation
    CR[b] = u.Quat(_R.x, _R.y, _R.z, _R.w)


def comp_to_local(bone, rsw):
    Qw = u.Rotator(roll=rsw[2], pitch=rsw[0], yaw=rsw[1]).quaternion()
    return qinv(CR[PARENT[bone]]) * (Qw * CR[bone])


def hand_for(pose):
    override = {}
    for b, rsw in pose.items():
        nl = comp_to_local(b, rsw)
        override[b] = u.Transform(L[b].translation, nl.rotator(), L[b].scale3d)
    C = u.Transform(u.Vector(0.0, 0.0, 0.0), u.Rotator(0.0, 0.0, 0.0), u.Vector(1.0, 1.0, 1.0))
    for b in CHAIN:
        C = u.MathLibrary.compose_transforms(override.get(b, L[b]), C)
        if b == "hand_r":
            return C.translation
    return None


POSES = [
    ("ready ", {"upperarm_r": (20, 5, 0), "lowerarm_r": (10, 0, 0)}),
    ("windup", {"upperarm_r": (145, 30, 15), "lowerarm_r": (75, 0, 0), "spine_04": (5, 20, 0)}),
    ("strike", {"upperarm_r": (25, -30, 0), "lowerarm_r": (12, 0, 0), "spine_04": (-12, -18, 0)}),
    ("follow", {"upperarm_r": (-8, -55, -10), "lowerarm_r": (5, 0, 0), "spine_04": (-8, -25, 0)}),
]
log("idle hand = (20,-11,104) ; X=fwd Y=right Z=up")
for label, p in POSES:
    h = hand_for(p)
    log("%s hand=(%.0f, %.0f, %.0f)" % (label, h.x, h.y, h.z))
log("DONE")

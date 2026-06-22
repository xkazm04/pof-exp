"""Numeric forward-kinematics calibration — no render ambiguity.

Compose the arm chain's local transforms to the hand's COMPONENT position, then apply test
component-space rotations to upperarm_r and read where the hand goes. Component frame:
X=forward, Y=right, Z=up. Tells us EXACTLY which rotation raises (hand Z up) / sweeps across
(hand Y) / etc., so we can author a clean slash.
"""
import unreal as u
T = "FKC"
def log(m): u.log("%s: %s" % (T, m))

IDLE = "/Game/Mixamo/Retargeted/SKM_Manny/Standard_Idle_RT"
CHAIN = ["root", "pelvis", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05",
         "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r"]
def qinv(q): return u.Quat(-q.x, -q.y, -q.z, q.w)

idle = u.load_asset(IDLE)
L = {b: u.AnimationLibrary.get_bone_pose_for_frame(idle, b, 0, False) for b in CHAIN}


def comp_chain(override=None):
    """Component transforms down the chain. child = compose_transforms(local, parent)."""
    C = u.Transform(u.Vector(0.0, 0.0, 0.0), u.Rotator(0.0, 0.0, 0.0), u.Vector(1.0, 1.0, 1.0))
    out = {}
    for b in CHAIN:
        loc = (override or {}).get(b, L[b])
        C = u.MathLibrary.compose_transforms(loc, C)
        out[b] = C
    return out


def hand_of(comps):
    return comps[comps and "hand_r"].translation


def main():
    base = comp_chain()
    for b in ["pelvis", "spine_05", "upperarm_r", "hand_r"]:
        t = base[b].translation
        log("idle %-11s comp=(%.1f, %.1f, %.1f)" % (b, t.x, t.y, t.z))

    h0 = base["hand_r"].translation
    PARENT = {"upperarm_r": "clavicle_r", "lowerarm_r": "upperarm_r",
              "spine_03": "spine_02", "spine_04": "spine_03"}

    def test(bone, label, raise_d, sweep_d, twist_d):
        Rp = base[PARENT[bone]].rotation
        Rc = base[bone].rotation
        Qw = u.Rotator(roll=float(twist_d), pitch=float(raise_d), yaw=float(sweep_d)).quaternion()
        new_local = qinv(Rp) * (Qw * Rc)
        newL = u.Transform(L[bone].translation, new_local.rotator(), L[bone].scale3d)
        h = comp_chain({bone: newL})["hand_r"].translation
        log("%-26s hand=(%.0f,%.0f,%.0f)  d=(%+.0f,%+.0f,%+.0f)" % (
            label, h.x, h.y, h.z, h.x - h0.x, h.y - h0.y, h.z - h0.z))

    log("--- upperarm_r ---")
    for r in [90, 135]:
        test("upperarm_r", "raise(+pitch %d)" % r, r, 0, 0)
    log("--- spine_04 (torso) : how it moves the hand ---")
    for s in [25, -25]:
        test("spine_04", "coil(yaw %d)" % s, 0, s, 0)
    for r in [20, -20]:
        test("spine_04", "lean(pitch %d)" % r, r, 0, 0)
    log("DONE")


main()

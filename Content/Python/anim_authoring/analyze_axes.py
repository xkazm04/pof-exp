"""Analytically calibrate bone rotation axes: compose local pose transforms up the mannequin
chain to get each bone's COMPONENT-space orientation, then report the world direction each
LOCAL axis (X/Y/Z) points. Rotating a bone about its local axis A rotates the limb about the
world direction of A — so this tells us exactly which local rotation raises/sweeps/twists.

Component frame (actor-relative): X=forward, Y=right, Z=up.
  raise/lower the arm (sagittal)  -> rotate about ~component Y (right)
  sweep across body (horizontal)  -> rotate about ~component Z (up)
  twist the limb                  -> rotate about the limb's own direction
Validation anchor: render showed upperarm_r local +X raises the arm up/forward => local X
world-dir should come out ~ +/-component-Y.
"""
import unreal as u

T = "AXES"
def log(m): u.log("%s: %s" % (T, m))

IDLE = "/Game/Mixamo/Retargeted/SKM_Manny/Standard_Idle_RT"
# Standard UE5 mannequin parent chain (root -> ... -> hand_r).
CHAIN = ["root", "pelvis", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05",
         "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r"]
REPORT = ["spine_03", "spine_04", "spine_05", "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r"]


def fmt(v):
    return "(%.2f, %.2f, %.2f)" % (v.x, v.y, v.z)


def main():
    idle = u.load_asset(IDLE)
    comp = {}
    Racc = u.Rotator(0.0, 0.0, 0.0).quaternion()  # identity (Quat() defaults to degenerate 0,0,0,0)
    for b in CHAIN:
        t = u.AnimationLibrary.get_bone_pose_for_frame(idle, b, 0, False)
        Racc = Racc * t.rotation                       # component = parent_component * local
        comp[b] = u.Quat(Racc.x, Racc.y, Racc.z, Racc.w)  # copy

    for b in REPORT:
        rot = comp[b].rotator()
        wx = u.MathLibrary.get_forward_vector(rot)     # world dir of local X
        wy = u.MathLibrary.get_right_vector(rot)       # world dir of local Y
        wz = u.MathLibrary.get_up_vector(rot)          # world dir of local Z
        log("%-11s  localX->%s  localY->%s  localZ->%s" % (b, fmt(wx), fmt(wy), fmt(wz)))
    log("DONE")


main()

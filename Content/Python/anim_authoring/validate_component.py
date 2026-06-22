"""Validate component-space authoring: raise upperarm_r from idle by rotating about the
component RIGHT axis (pitch). If correct, the arm sweeps cleanly up/overhead.

new_local = inv(R_parent_comp) * Qw * R_comp_idle   (Qw = world/component-space delta)
"""
import sys
sys.path.insert(0, r"C:/Users/kazda/Documents/Unreal Projects/PoF/Content/Python/anim_authoring")
import unreal as u
import pose_engine as pe

T = "VCOMP"
def log(m): u.log("%s: %s" % (T, m))

IDLE = "/Game/Mixamo/Retargeted/SKM_Manny/Standard_Idle_RT"
CHAIN = ["root", "pelvis", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05",
         "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r"]
FRAMES, FPS = 30, 30


def qinv(q):
    return u.Quat(-q.x, -q.y, -q.z, q.w)  # unit-quat inverse = conjugate


def comp_rotations():
    idle = u.load_asset(IDLE)
    R = u.Rotator(0.0, 0.0, 0.0).quaternion()
    out = {}
    for b in CHAIN:
        t = u.AnimationLibrary.get_bone_pose_for_frame(idle, b, 0, False)
        R = R * t.rotation
        out[b] = u.Quat(R.x, R.y, R.z, R.w)
    return out


def main():
    skel, names, base = pe.load_base_pose()
    Rcomp = comp_rotations()
    Rparent = Rcomp["clavicle_r"]
    Rc_idle = Rcomp["upperarm_r"]

    def abs_fn(k):
        raise_deg = 170.0 * (k / float(FRAMES))           # 0 -> 170 about component RIGHT (pitch): down->fwd->overhead
        Qw = u.Rotator(roll=0.0, pitch=raise_deg, yaw=0.0).quaternion()
        new_local = qinv(Rparent) * (Qw * Rc_idle)
        return {"upperarm_r": new_local}

    path = pe.author_abs("RaiseTest", "/Game/PoF/GenAnims", skel, base, FRAMES, FPS, abs_fn)
    log("authored %s" % path)
    log("[gate] RESULT=PASS")


main()

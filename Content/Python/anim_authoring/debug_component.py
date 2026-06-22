import unreal as u
T = "DCOMP"
def log(m): u.log("%s: %s" % (T, m))

IDLE = "/Game/Mixamo/Retargeted/SKM_Manny/Standard_Idle_RT"
CHAIN = ["root", "pelvis", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05",
         "clavicle_r", "upperarm_r", "lowerarm_r"]
def qinv(q): return u.Quat(-q.x, -q.y, -q.z, q.w)

idle = u.load_asset(IDLE)
Racc = u.Rotator(0.0, 0.0, 0.0).quaternion()
comp = {}
for b in CHAIN:
    lr = u.AnimationLibrary.get_bone_pose_for_frame(idle, b, 0, False).rotation
    Racc = Racc * lr
    comp[b] = u.Quat(Racc.x, Racc.y, Racc.z, Racc.w)
Rp, Rc = comp["clavicle_r"], comp["upperarm_r"]

# does the formula vary with the raise delta?
for rd in [0, 45, 90]:
    Qw = u.Rotator(roll=0.0, pitch=float(rd), yaw=0.0).quaternion()
    nl = qinv(Rp) * (Qw * Rc)
    log("formula raise=%-3d -> new_local %s" % (rd, nl.rotator()))

# did the authored RaiseTest store varying keys?
rt = u.load_asset("/Game/PoF/GenAnims/RaiseTest")
if rt:
    for f in [0, 15, 30]:
        r = u.AnimationLibrary.get_bone_pose_for_frame(rt, "upperarm_r", f, False).rotation
        log("RaiseTest f%-2d upperarm_r=%s" % (f, r.rotator()))
log("DONE")

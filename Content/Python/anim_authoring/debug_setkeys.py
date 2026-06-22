import unreal as u
T = "SETK"
def log(m): u.log("%s: %s" % (T, m))

idle = u.load_asset("/Game/Mixamo/Retargeted/SKM_Manny/Standard_Idle_RT")
skel = idle.get_skeleton()
bt = u.AnimationLibrary.get_bone_pose_for_frame(idle, "upperarm_r", 0, False)
bpos, brot, bscale = bt.translation, bt.rotation, bt.scale3d
log("base upperarm_r rot=%s" % brot.rotator())

# 1) does euler->quat vary?
for e in [(0, 0, 0), (90, 0, 0), (0, 90, 0), (0, 0, 90)]:
    q = u.Rotator(roll=e[0], pitch=e[1], yaw=e[2]).quaternion()
    log("euler %s -> quat.rotator %s" % (e, q.rotator()))

# 2) does brot * delta vary?
d0 = u.Rotator(roll=0, pitch=0, yaw=0).quaternion()
d90 = u.Rotator(roll=90, pitch=0, yaw=0).quaternion()
log("brot*d0   = %s" % (brot * d0).rotator())
log("brot*d90  = %s" % (brot * d90).rotator())

# 3) write a sweep and read it back
path = "/Game/PoF/GenAnims/KeyTest"
if u.EditorAssetLibrary.does_asset_exist(path):
    u.EditorAssetLibrary.delete_asset(path)
f = u.AnimSequenceFactory(); f.set_editor_property("target_skeleton", skel)
anim = u.AssetToolsHelpers.get_asset_tools().create_asset("KeyTest", "/Game/PoF/GenAnims", u.AnimSequence, f)
ctrl = anim.controller
log("controller type=%s has_set_keys=%s" % (type(ctrl).__name__, hasattr(ctrl, "set_bone_track_keys")))
log("controller methods: %s" % ", ".join(m for m in dir(ctrl) if any(k in m.lower() for k in ['bone','key','frame','bracket','rate'])))
ctrl.open_bracket("t", True)
ctrl.set_frame_rate(u.FrameRate(30, 1))
ctrl.set_number_of_frames(u.FrameNumber(30))
pk, rk, sk = [], [], []
for k in range(31):
    ang = 90.0 * k / 30.0
    r = brot * u.Rotator(roll=ang, pitch=0, yaw=0).quaternion()
    pk.append(u.Vector(bpos.x, bpos.y, bpos.z)); rk.append(r); sk.append(u.Vector(1, 1, 1))
log("built rk[0]=%s rk[15]=%s rk[30]=%s" % (rk[0].rotator(), rk[15].rotator(), rk[30].rotator()))
added = ctrl.add_bone_track("upperarm_r")
log("add_bone_track -> %s" % added)
ok = ctrl.set_bone_track_keys("upperarm_r", pk, rk, sk, True)
log("set_keys 31 -> %s" % ok)
if not ok:
    ok = ctrl.set_bone_track_keys("upperarm_r", pk[:30], rk[:30], sk[:30], True)
    log("set_keys 30 -> %s" % ok)
ctrl.close_bracket(True)
u.EditorAssetLibrary.save_asset(path)
for fr in [0, 15, 30]:
    t = u.AnimationLibrary.get_bone_pose_for_frame(anim, "upperarm_r", fr, False)
    log("readback f%d = %s" % (fr, t.rotation.rotator()))
log("DONE")

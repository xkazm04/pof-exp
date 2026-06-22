import unreal as u
T = "FK"
def log(m): u.log("%s: %s" % (T, m))

rt = u.load_asset("/Game/PoF/GenAnims/RaiseTest")   # upperarm_r raise 0..170 across 30 frames
NAMES = ["pelvis", "upperarm_r", "lowerarm_r", "hand_r"]

# Is get_bone_poses_for_frame component-space? Read hand height across the raise sweep.
for f in [0, 10, 20, 30]:
    try:
        poses = u.AnimationLibrary.get_bone_poses_for_frame(rt, NAMES, f, False)
        hand = poses[NAMES.index("hand_r")].translation
        pelv = poses[NAMES.index("pelvis")].translation
        log("f%-2d hand=(%.1f, %.1f, %.1f)  pelvis=(%.1f, %.1f, %.1f)" % (
            f, hand.x, hand.y, hand.z, pelv.x, pelv.y, pelv.z))
    except Exception as e:
        log("get_bone_poses_for_frame ERR: %s" % e)
        break
log("DONE")

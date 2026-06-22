import unreal
T = "SWKEYS"
def log(m): unreal.log("%s: %s" % (T, m))

a = unreal.load_asset("/Game/PoF/GenAnims/SwordSlash01")
log("play_length=%s num_frames(model)=%s" % (a.get_play_length(),
    a.data_model_interface.get_number_of_frames()))
for f in [0, 7, 13, 19, 30]:
    tu = unreal.AnimationLibrary.get_bone_pose_for_frame(a, "upperarm_r", f, False)
    tl = unreal.AnimationLibrary.get_bone_pose_for_frame(a, "lowerarm_r", f, False)
    log("f%-2d upperarm_r=%s  lowerarm_r=%s" % (
        f, tu.rotation.rotator(), tl.rotation.rotator()))
log("DONE")

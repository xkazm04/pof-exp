import unreal
T = "READDBG"
def log(m): unreal.log("%s: %s" % (T, m))

idle = unreal.load_asset("/Game/Mixamo/Retargeted/SKM_Manny/Standard_Idle_RT")
model = idle.data_model_interface
names = list(model.get_bone_track_names())
log("get_bone_track_names count=%d" % len(names))
log("first=%s" % ", ".join(str(n) for n in names[:10]))

# model API surface
log("model API: %s" % ", ".join(m for m in dir(model) if any(k in m.lower() for k in ['track','bone','pose','eval','transform'])))

n0 = None
for n in names:
    if str(n) == "upperarm_r":
        n0 = n; break
n0 = n0 or (names[0] if names else None)
log("probe bone = %s" % n0)

if n0 is not None:
    # 1) get_raw_track_data
    try:
        d = unreal.AnimationLibrary.get_raw_track_data(idle, n0)
        log("get_raw_track_data type=%s len=%s" % (type(d).__name__, len(d) if hasattr(d,'__len__') else '?'))
        try:
            log("  [0] type=%s len=%s" % (type(d[0]).__name__, len(d[0]) if hasattr(d[0],'__len__') else '?'))
        except Exception as e: log("  unpack err %s" % e)
    except Exception as e:
        log("get_raw_track_data ERR: %s" % e)

    # 2) get_bone_pose_for_frame (evaluated, local)
    for fn in ["get_bone_pose_for_frame"]:
        try:
            t = getattr(unreal.AnimationLibrary, fn)(idle, n0, 0, False)
            log("%s -> T=%s R=%s S=%s" % (fn, t.translation, t.rotation.rotator(), t.scale3d))
        except Exception as e:
            log("%s ERR: %s" % (fn, e))

    # 3) model.get_bone_track_keys variants
    for fn in ["get_bone_track_keys", "evaluate_bone_track_transform", "get_bone_pose_for_frame"]:
        if hasattr(model, fn):
            log("model has %s" % fn)

# 4) AnimationLibrary surface for reading
log("AnimLib readers: %s" % ", ".join(m for m in dir(unreal.AnimationLibrary) if any(k in m.lower() for k in ['raw','pose','bone_track','transform'])))
log("DONE")

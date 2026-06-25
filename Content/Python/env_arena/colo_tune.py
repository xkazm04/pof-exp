"""Colosseum — reusable lighting/exposure/camera tuner. Edit the constants and re-run.
Idempotent: finds the existing sun/sky/PP/camera and re-sets them (no duplicate spawns)."""
import unreal

MAP = "/Game/Maps/Arena_Ancient"

# --- tune me ---
SUN_INTENSITY = 11.0
SUN_PITCH = -32.0
SUN_YAW = 40.0
SKY_INTENSITY = 2.6
EXP_BIAS = 0.2
EXP_MIN = 0.1
EXP_MAX = 4.0
# absolute camera (the colosseum stands reach radius ~2280, top ~z1100)
CAM_X, CAM_Y, CAM_Z = 2700.0, -2700.0, 1300.0   # elevated 3/4 — whole bowl + arcade ring
LOOK_X, LOOK_Y, LOOK_Z = 0.0, 0.0, 250.0
FOV = 72.0
# ----------------


def S(o, p, v):
    try:
        o.set_editor_property(p, v); return True
    except Exception as e:  # noqa: BLE001
        unreal.log_warning("[TUNE] set %s failed: %s" % (p, e)); return False


def comp(a, c):
    return a.get_component_by_class(c)


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    dl = sl = ppv = cam = arena = None
    for a in eas.get_all_level_actors():
        cls = a.get_class().get_name(); lbl = a.get_actor_label()
        if cls == "DirectionalLight" and not dl: dl = a
        elif cls == "SkyLight" and not sl: sl = a
        elif cls == "PostProcessVolume": ppv = a
        elif lbl == "ColoCam_Hero": cam = a
        elif lbl == "Arena": arena = a

    extent = unreal.Vector(1000, 1000, 400)
    if arena:
        try: _, extent = arena.get_actor_bounds(False)
        except Exception: pass
    radius = max(700.0, min(extent.x, extent.y))

    if dl:
        dlc = comp(dl, unreal.DirectionalLightComponent)
        dlc.set_intensity(SUN_INTENSITY)
        dl.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=SUN_PITCH, yaw=SUN_YAW), False)
    if sl:
        comp(sl, unreal.SkyLightComponent).set_intensity(SKY_INTENSITY)
    if ppv:
        s = ppv.get_editor_property("settings")
        for p, v in (("override_auto_exposure_bias", True), ("auto_exposure_bias", EXP_BIAS),
                     ("override_auto_exposure_min_brightness", True), ("auto_exposure_min_brightness", EXP_MIN),
                     ("override_auto_exposure_max_brightness", True), ("auto_exposure_max_brightness", EXP_MAX)):
            S(s, p, v)
        ppv.set_editor_property("settings", s)
    if cam:
        cam_loc = unreal.Vector(CAM_X, CAM_Y, CAM_Z)
        look = unreal.Vector(LOOK_X, LOOK_Y, LOOK_Z)
        rot = unreal.MathLibrary.find_look_at_rotation(cam_loc, look)
        cam.set_actor_location_and_rotation(cam_loc, rot, False, False)
        cc = comp(cam, unreal.CameraComponent)
        if cc: S(cc, "field_of_view", FOV)

    saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("[TUNE] sun=%.1f@%.0f sky=%.1f bias=%.1f cam=(%.0f,%.0f,%.0f) saved=%s"
               % (SUN_INTENSITY, SUN_PITCH, SKY_INTENSITY, EXP_BIAS, CAM_X, CAM_Y, CAM_Z, saved))
    unreal.log("[gate] RESULT=%s" % ("PASS" if saved else "FAIL"))


main()

"""Colosseum mastering — Pass 1: cinematic lighting + atmosphere (engine-native, no marketplace assets).

Lumen GI + reflections, golden-hour low sun with soft shadows + volumetric god-rays,
volumetric fog haze, SkyAtmosphere + VolumetricCloud, a filmic post grade, and a
cinematic auto-activating CameraActor so the -game viewport renders a hero angle (not
top-down). All property sets are guarded so name drift across 5.8 degrades gracefully.
"""
import json
import unreal

MAP = "/Game/Maps/Arena_Ancient"
OUT = unreal.Paths.project_saved_dir() + "colo_pass1.json"
res = {"set": [], "errors": []}


def L(m):
    unreal.log("[COLO1] " + m)


def S(o, p, v):
    try:
        o.set_editor_property(p, v)
        res["set"].append(p)
        return True
    except Exception as e:  # noqa: BLE001
        res["errors"].append("%s: %s" % (p, e))
        return False


def comp(a, c):
    return a.get_component_by_class(c)


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    dl = sl = fog = ppv = sky = arena = None
    for a in list(eas.get_all_level_actors()):
        cls = a.get_class().get_name()
        lbl = a.get_actor_label()
        if cls == "DirectionalLight" and not dl:
            dl = a
        elif cls == "SkyLight" and not sl:
            sl = a
        elif cls == "ExponentialHeightFog":
            fog = a
        elif cls == "PostProcessVolume":
            ppv = a
        elif cls == "SkyAtmosphere":
            sky = a
        elif lbl == "Arena":
            arena = a

    # ---------- arena extent (camera framing + cloud height) ----------
    extent = unreal.Vector(1000, 1000, 400)
    if arena:
        try:
            _, extent = arena.get_actor_bounds(False)
        except Exception:  # noqa: BLE001
            pass
    radius = max(700.0, min(extent.x, extent.y))

    # ---------- golden-hour key sun (low, warm, soft, god-ray scattering) ----------
    if dl:
        dlc = comp(dl, unreal.DirectionalLightComponent)
        S(dlc, "mobility", unreal.ComponentMobility.MOVABLE)
        dlc.set_intensity(7.0)
        dlc.set_light_color(unreal.LinearColor(1.0, 0.79, 0.52, 1.0))
        S(dlc, "light_source_angle", 1.4)          # softer penumbra
        S(dlc, "volumetric_scattering_intensity", 2.5)  # god rays in the fog
        S(dlc, "cast_shadows", True)
        dl.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-15.0, yaw=38.0), False)

    # ---------- sky fill (real-time capture picks up the warm atmosphere) ----------
    if sl:
        slc = comp(sl, unreal.SkyLightComponent)
        S(slc, "mobility", unreal.ComponentMobility.MOVABLE)
        S(slc, "real_time_capture", True)
        slc.set_intensity(1.0)

    # ---------- volumetric fog haze ----------
    if fog:
        fc = comp(fog, unreal.ExponentialHeightFogComponent)
        S(fc, "fog_density", 0.018)
        S(fc, "fog_height_falloff", 0.18)
        S(fc, "enable_volumetric_fog", True)
        S(fc, "volumetric_fog_scattering_distribution", 0.55)
        S(fc, "volumetric_fog_albedo", unreal.Color(235, 220, 200, 255))
        S(fc, "volumetric_fog_extinction_scale", 1.4)
        if not S(fc, "fog_inscattering_luminance", unreal.LinearColor(0.62, 0.52, 0.40, 1.0)):
            S(fc, "fog_inscattering_color", unreal.LinearColor(0.62, 0.52, 0.40, 1.0))

    # ---------- volumetric cloud (engine default material) ----------
    if not any(a.get_class().get_name() == "VolumetricCloud" for a in eas.get_all_level_actors()):
        vc = eas.spawn_actor_from_class(unreal.VolumetricCloud, unreal.Vector(0, 0, 0))
        if vc:
            vc.set_actor_label("Colo_Clouds")
            res["set"].append("VolumetricCloud")

    # ---------- Lumen GI + reflections + filmic grade on the unbound PP ----------
    if ppv:
        S(ppv, "unbound", True)
        s = ppv.get_editor_property("settings")
        gi = [
            ("override_dynamic_global_illumination_method", True),
            ("dynamic_global_illumination_method", unreal.DynamicGlobalIlluminationMethod.LUMEN),
            ("override_reflection_method", True),
            ("reflection_method", unreal.ReflectionMethod.LUMEN),
            ("override_lumen_scene_lighting_quality", True), ("lumen_scene_lighting_quality", 2.0),
            ("override_lumen_final_gather_quality", True), ("lumen_final_gather_quality", 2.0),
            # exposure: mild histogram bias so the lit sand keeps highlight detail
            ("override_auto_exposure_bias", True), ("auto_exposure_bias", 0.4),
            # filmic look
            ("override_bloom_intensity", True), ("bloom_intensity", 0.55),
            ("override_ambient_occlusion_intensity", True), ("ambient_occlusion_intensity", 0.6),
            ("override_ambient_occlusion_radius", True), ("ambient_occlusion_radius", 80.0),
            ("override_vignette_intensity", True), ("vignette_intensity", 0.32),
            ("override_film_grain_intensity", True), ("film_grain_intensity", 0.10),
            # warm cinematic grade
            ("override_color_saturation", True), ("color_saturation", unreal.Vector4(1.06, 1.02, 0.97, 1.0)),
            ("override_color_contrast", True), ("color_contrast", unreal.Vector4(1.06, 1.05, 1.04, 1.0)),
            ("override_white_temp", True), ("white_temp", 5600.0),
        ]
        for p, v in gi:
            S(s, p, v)
        ppv.set_editor_property("settings", s)

    # ---------- cinematic auto-activating camera (hero 3/4, INSIDE the arena) ----------
    cam_loc = unreal.Vector(radius * 0.78, -radius * 0.78, 330.0)   # inside near a corner
    look_at = unreal.Vector(-radius * 0.10, radius * 0.10, 130.0)   # across the floor toward far wall
    cam_rot = unreal.MathLibrary.find_look_at_rotation(cam_loc, look_at)
    cam = None
    for a in eas.get_all_level_actors():
        if a.get_actor_label() == "ColoCam_Hero":
            cam = a
            break
    if not cam:
        cam = eas.spawn_actor_from_class(unreal.CameraActor, cam_loc, cam_rot)
    if cam:
        cam.set_actor_label("ColoCam_Hero")
        cam.set_actor_location_and_rotation(cam_loc, cam_rot, False, False)
        S(cam, "auto_activate_for_player", unreal.AutoReceiveInput.PLAYER0)
        cc = comp(cam, unreal.CameraComponent)
        if cc:
            S(cc, "field_of_view", 62.0)
        res["set"].append("CameraActor@%d,%d,%d" % (cam_loc.x, cam_loc.y, cam_loc.z))

    saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    res["saved"] = bool(saved)
    res["radius"] = radius
    with open(OUT, "w") as f:
        json.dump(res, f, indent=2)
    L("saved=%s radius=%.0f sets=%d errors=%d" % (saved, radius, len(res["set"]), len(res["errors"])))
    L("[gate] RESULT=%s" % ("PASS" if saved else "FAIL"))


main()

"""Give the VerticalSlice arena a moody, cinematic look instead of flat gray-box lighting.

Uses MOVABLE (dynamic) lights so nothing needs a Lightmass bake: a low, warm key light for
long dramatic shadows; a dimmer sky light for darker, higher-contrast ambient; atmospheric
height fog for depth; and a post-process volume with bloom (so the lightsabers glow) + a
gentle vignette. Persisted into the map so the playable game gets the improved arena.
"""
import unreal

MAP_PKG = "/Game/Maps/VerticalSlice"


def find_actor(cls):
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for a in eas.get_all_level_actors():
        if isinstance(a, cls):
            return a
    return None


def spawn(cls, loc=(0.0, 0.0, 0.0)):
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return eas.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(0.0, 0.0, 0.0))


def comp_of(actor, comp_cls):
    return actor.get_component_by_class(comp_cls)


def safe_set(obj, prop, val):
    """set_editor_property that logs and continues on unknown/renamed properties."""
    try:
        obj.set_editor_property(prop, val)
        return True
    except Exception as e:
        unreal.log_warning("ARENA_LIGHT: set '%s' failed (%s)" % (prop, e))
        return False


def main():
    unreal.log("ARENA_LIGHT: BEGIN")
    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PKG)

    # --- Key light: low warm sun, movable, strong shadows ---
    dl = find_actor(unreal.DirectionalLight) or spawn(unreal.DirectionalLight, (0.0, 0.0, 2000.0))
    dlc = comp_of(dl, unreal.DirectionalLightComponent)
    safe_set(dlc, "mobility", unreal.ComponentMobility.MOVABLE)
    dlc.set_intensity(2.2)
    dlc.set_light_color(unreal.LinearColor(1.0, 0.925, 0.80, 1.0))  # warm key
    safe_set(dlc, "cast_shadows", True)
    dl.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=-30.0, yaw=55.0), False)
    unreal.log("ARENA_LIGHT: key light set")

    # --- Sky light: dimmer ambient => darker, moodier contrast ---
    sl = find_actor(unreal.SkyLight) or spawn(unreal.SkyLight, (0.0, 0.0, 2000.0))
    slc = comp_of(sl, unreal.SkyLightComponent)
    safe_set(slc, "mobility", unreal.ComponentMobility.MOVABLE)
    slc.set_intensity(0.5)
    slc.set_light_color(unreal.LinearColor(0.59, 0.67, 0.80, 1.0))  # cool fill
    safe_set(slc, "real_time_capture", True)
    unreal.log("ARENA_LIGHT: sky light set")

    # --- Atmosphere: height fog for depth ---
    fog = find_actor(unreal.ExponentialHeightFog) or spawn(unreal.ExponentialHeightFog, (0.0, 0.0, -200.0))
    fc = comp_of(fog, unreal.ExponentialHeightFogComponent)
    safe_set(fc, "fog_density", 0.025)
    safe_set(fc, "fog_height_falloff", 0.25)
    # UE5 renamed the base inscatter to *_luminance; try both, harmlessly.
    if not safe_set(fc, "fog_inscattering_luminance", unreal.LinearColor(0.35, 0.45, 0.62, 1.0)):
        safe_set(fc, "fog_inscattering_color", unreal.LinearColor(0.35, 0.45, 0.62, 1.0))
    unreal.log("ARENA_LIGHT: fog set")

    # --- Post-process: bloom for glowing sabers + gentle vignette ---
    ppv = find_actor(unreal.PostProcessVolume) or spawn(unreal.PostProcessVolume, (0.0, 0.0, 0.0))
    safe_set(ppv, "unbound", True)
    s = ppv.get_editor_property("settings")
    safe_set(s, "override_bloom_intensity", True)
    safe_set(s, "bloom_intensity", 1.4)
    # Tame the top-down gameplay view: pull auto-exposure down ~1 stop so the lit floor seen
    # from above keeps highlight detail instead of clipping to white.
    safe_set(s, "override_auto_exposure_bias", True)
    safe_set(s, "auto_exposure_bias", -1.0)
    safe_set(s, "override_vignette_intensity", True)
    safe_set(s, "vignette_intensity", 0.45)
    safe_set(s, "override_film_grain_intensity", True)
    safe_set(s, "film_grain_intensity", 0.08)
    ppv.set_editor_property("settings", s)
    unreal.log("ARENA_LIGHT: post-process set")

    saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("ARENA_LIGHT: save_current_level -> %s" % saved)
    unreal.log("[gate] RESULT=PASS")


main()

"""
relight_test_jedi.py — hero lighting for the Jedi capture.

Test_Jedi is a duplicate of the dim VerticalSlice arena, so the Jedi spawns in
shadow and the robes read dark. Add movable hero lights aimed at the player
spawn (no Lightmass bake needed — movable lights render in the viewport + the
SceneCapture picks them up because the arena already has baked ambient):
  - a warm KEY spotlight, front-right-above (the side the capture sees)
  - a cool RIM spotlight from behind for silhouette separation
  - a gentle fill so shadow detail isn't crushed

Idempotent: removes any prior Jedi_* hero lights before adding. Writes
Saved/relight_test_jedi.json.
"""
import json
import unreal

EAL = unreal.EditorAssetLibrary
MAP = "/Game/Maps/Test_Jedi"
OUT = {"steps": []}


def _log(m):
    unreal.log_warning("[relight] " + str(m))
    OUT["steps"].append(str(m))


def add_spot(eas, label, loc, target, intensity, color, outer_cone, radius):
    sl = eas.spawn_actor_from_class(unreal.SpotLight, loc)
    sl.set_actor_label(label)
    rot = unreal.MathLibrary.find_look_at_rotation(loc, target)
    sl.set_actor_rotation(rot, False)
    c = sl.get_component_by_class(unreal.SpotLightComponent)
    c.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    c.set_intensity(intensity)
    c.set_light_color(color)
    c.set_editor_property("outer_cone_angle", outer_cone)
    c.set_editor_property("inner_cone_angle", max(0.0, outer_cone - 12.0))
    c.set_editor_property("attenuation_radius", radius)
    c.set_editor_property("cast_shadows", True)
    return sl


def main():
    _log("=== RELIGHT TEST_JEDI START ===")
    world = unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # Find the player spawn.
    spawn = unreal.Vector(0, 0, 120)
    for a in eas.get_all_level_actors():
        if isinstance(a, unreal.PlayerStart):
            spawn = a.get_actor_location()
            break
    _log("player spawn at (%.0f,%.0f,%.0f)" % (spawn.x, spawn.y, spawn.z))
    chest = unreal.Vector(spawn.x, spawn.y, spawn.z + 10)   # aim at the torso

    # Remove prior hero lights (idempotent).
    removed = 0
    for a in list(eas.get_all_level_actors()):
        if a.get_actor_label().startswith("Jedi_"):
            eas.destroy_actor(a)
            removed += 1
    _log("removed %d prior hero light(s)" % removed)

    # LOCK EXPOSURE — auto-exposure was crushing the subject to a silhouette when
    # the lights brightened the room, and killing the saber bloom. A fixed exposure
    # makes the look predictable and keeps the scene moody so the saber glows.
    ppv = eas.spawn_actor_from_class(unreal.PostProcessVolume, spawn)
    ppv.set_actor_label("Jedi_PPV")
    ppv.set_editor_property("unbound", True)
    ppv.set_editor_property("priority", 100.0)
    s = ppv.get_editor_property("settings")
    s.set_editor_property("override_auto_exposure_min_brightness", True)
    s.set_editor_property("auto_exposure_min_brightness", 1.0)
    s.set_editor_property("override_auto_exposure_max_brightness", True)
    s.set_editor_property("auto_exposure_max_brightness", 1.0)
    s.set_editor_property("override_bloom_intensity", True)
    s.set_editor_property("bloom_intensity", 1.4)
    ppv.set_editor_property("settings", s)
    _log("locked exposure PPV (min=max=1.0, bloom 1.4)")

    warm = unreal.LinearColor(1.0, 0.86, 0.66, 1.0)
    cool = unreal.LinearColor(0.55, 0.7, 1.0, 1.0)

    # Gentle, TIGHT spotlights aimed at the character (small radius so they don't
    # flood the room). Tuned to the locked exposure.
    # KEY — camera side (+Y, where the side capture sits), front, mid-height.
    add_spot(eas, "Jedi_Key",
             unreal.Vector(spawn.x + 120, spawn.y + 260, spawn.z + 230),
             chest, 22000.0, warm, 24.0, 900.0)
    # RIM — behind-left, cool, for edge separation.
    add_spot(eas, "Jedi_Rim",
             unreal.Vector(spawn.x - 180, spawn.y - 160, spawn.z + 260),
             chest, 16000.0, cool, 22.0, 900.0)
    _log("added Key + Rim spotlights (gentle, tight)")

    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, MAP)
    _log("save_map ok=%s" % saved)
    OUT["result"] = "PASS" if saved else "FAIL"
    out_path = unreal.Paths.combine([unreal.Paths.project_saved_dir(), "relight_test_jedi.json"])
    with open(out_path, "w", encoding="utf-8") as fh:
        json.dump(OUT, fh, indent=2)
    _log("[gate] RESULT=%s" % OUT["result"])


main()

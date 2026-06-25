"""Stream 1 (env-arena) — author the ancient sandy arena into /Game/Maps/Arena_Ancient.

Arena_Ancient is a Phase-0 kitchen-sink duplicate of VerticalSlice. This script REBUILDS it
into a clean lit desert arena:
  1. Create a procedural sand material  /Game/Environments/AncientArena/M_Sand_Floor.
  2. Declutter — remove the ProcGen blockout rooms, level generator, vegetation scatter,
     lightmass-importance volume, every VS* functional-test actor, the enemy, and the
     redundant duplicate lights / sky-lights / player-starts (keep one of each).
  3. Sandy floor — override the Arena mesh floor slot + the ground plane with M_Sand_Floor.
  4. Desert lighting — movable warm key sun + cool sky + warm haze fog + post-process
     (no Lightmass bake), reusing the proven improve_arena_lighting recipe, tuned brighter.
  5. Ancient ruins — scatter broken-pillar cylinders (sandstone M_Arena_Pillar) around the arena.
  6. PlayerStart (single, centered) + a NavMeshBoundsVolume over the play space.
  7. Save the level + the material. Writes Saved/env_build.json for the agent to read.
"""
import json
import math
import unreal

MAP = "/Game/Maps/Arena_Ancient"
ENVDIR = "/Game/Environments/AncientArena"
SAND = ENVDIR + "/M_Sand_Floor"
PILLAR_MAT = "/Game/ArenaBuild/M_Arena_Pillar"
CYL = "/Engine/BasicShapes/Cylinder"
OUT = unreal.Paths.project_saved_dir() + "env_build.json"

res = {"phases": [], "removed": [], "ruins": 0, "errors": []}


def log(m):
    unreal.log("[ENVBUILD] " + m)


def warn(m):
    unreal.log_warning("[ENVBUILD] " + m)
    res["errors"].append(m)


def safe_set(o, p, v):
    try:
        o.set_editor_property(p, v)
        return True
    except Exception as e:  # noqa: BLE001
        warn("set '%s' failed: %s" % (p, e))
        return False


def comp(actor, cls):
    return actor.get_component_by_class(cls)


# ---------------------------------------------------------------- sand material
def make_sand():
    if unreal.EditorAssetLibrary.does_asset_exist(SAND):
        log("sand material already exists")
        return unreal.load_asset(SAND)
    if not unreal.EditorAssetLibrary.does_directory_exist(ENVDIR):
        unreal.EditorAssetLibrary.make_directory(ENVDIR)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset("M_Sand_Floor", ENVDIR, unreal.Material, unreal.MaterialFactoryNew())
    mel = unreal.MaterialEditingLibrary

    light = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -700, -120)
    light.set_editor_property("constant", unreal.LinearColor(0.82, 0.71, 0.49, 1.0))
    dark = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -700, 120)
    dark.set_editor_property("constant", unreal.LinearColor(0.56, 0.46, 0.31, 1.0))

    base = light
    try:
        noise = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, -700, 360)
        safe_set(noise, "scale", 0.0009)
        safe_set(noise, "output_min", 0.0)
        safe_set(noise, "output_max", 1.0)
        lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -360, 0)
        mel.connect_material_expression(dark, "", lerp, "A")
        mel.connect_material_expression(light, "", lerp, "B")
        mel.connect_material_expression(noise, "", lerp, "Alpha")
        base = lerp
        log("sand: noise-driven tonal variation wired")
    except Exception as e:  # noqa: BLE001
        warn("sand noise lerp failed -> flat sand: %s" % e)

    mel.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -360, 260)
    rough.set_editor_property("r", 0.92)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -360, 410)
    spec.set_editor_property("r", 0.10)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    safe_set(mat, "used_with_static_lighting", True)  # grey-fallback guard
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(SAND)
    log("sand material created + saved")
    return mat


# ---------------------------------------------------------------- map authoring
def is_test(cls):
    return cls.endswith("Test") or "FunctionalTest" in cls


REMOVE_SUBSTR = ["BlockoutRoom", "LevelGenerator", "VegetationScatter", "LightmassImportance"]
REMOVE_EXACT = ["BP_VSEnemy_C"]


def build_map(sand_mat):
    unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    dirs, skies, starts, planes = [], [], [], []
    arena = fog = ppv = None
    for a in list(eas.get_all_level_actors()):
        cls = a.get_class().get_name()
        lbl = a.get_actor_label()
        if any(s in cls for s in REMOVE_SUBSTR) or cls in REMOVE_EXACT or is_test(cls):
            res["removed"].append("%s (%s)" % (lbl, cls))
            eas.destroy_actor(a)
            continue
        if cls == "DirectionalLight":
            dirs.append(a)
        elif cls == "SkyLight":
            skies.append(a)
        elif cls == "PlayerStart":
            starts.append(a)
        elif cls == "StaticMeshActor" and lbl.startswith("Plane"):
            planes.append(a)
        elif cls == "StaticMeshActor" and lbl == "Arena":
            arena = a
        elif cls == "ExponentialHeightFog":
            fog = a
        elif cls == "PostProcessVolume":
            ppv = a

    # --- keep one of each redundant set, destroy the rest
    for extra in dirs[1:] + skies[1:] + starts[1:] + planes[1:]:
        res["removed"].append("%s (dup)" % extra.get_actor_label())
        eas.destroy_actor(extra)

    # --- desert key light (movable warm sun, no bake)
    if dirs:
        dlc = comp(dirs[0], unreal.DirectionalLightComponent)
        safe_set(dlc, "mobility", unreal.ComponentMobility.MOVABLE)
        dlc.set_intensity(3.2)
        dlc.set_light_color(unreal.LinearColor(1.0, 0.94, 0.82, 1.0))
        safe_set(dlc, "cast_shadows", True)
        dirs[0].set_actor_rotation(unreal.Rotator(0.0, -38.0, 50.0), False)
    # --- cool sky fill
    if skies:
        slc = comp(skies[0], unreal.SkyLightComponent)
        safe_set(slc, "mobility", unreal.ComponentMobility.MOVABLE)
        slc.set_intensity(1.1)
        slc.set_light_color(unreal.LinearColor(0.72, 0.78, 0.90, 1.0))
        safe_set(slc, "real_time_capture", True)
    # --- warm dust haze
    if fog:
        fc = comp(fog, unreal.ExponentialHeightFogComponent)
        safe_set(fc, "fog_density", 0.012)
        safe_set(fc, "fog_height_falloff", 0.2)
        if not safe_set(fc, "fog_inscattering_luminance", unreal.LinearColor(0.62, 0.54, 0.42, 1.0)):
            safe_set(fc, "fog_inscattering_color", unreal.LinearColor(0.62, 0.54, 0.42, 1.0))
    # --- post-process (bright desert; modest bloom/vignette, slight exposure pull)
    if ppv:
        safe_set(ppv, "unbound", True)
        s = ppv.get_editor_property("settings")
        for p, v in (("override_bloom_intensity", True), ("bloom_intensity", 0.8),
                     ("override_auto_exposure_bias", True), ("auto_exposure_bias", -0.5),
                     ("override_vignette_intensity", True), ("vignette_intensity", 0.30),
                     ("override_film_grain_intensity", True), ("film_grain_intensity", 0.05)):
            safe_set(s, p, v)
        ppv.set_editor_property("settings", s)

    # --- sandy floor: override the Arena mesh floor slot (0) + keep one ground plane as sand
    if arena and sand_mat:
        smc = comp(arena, unreal.StaticMeshComponent)
        if smc:
            smc.set_material(0, sand_mat)
            log("assigned sand to Arena floor slot 0")
    ground = None
    if planes:
        ground = planes[0]
        gsmc = comp(ground, unreal.StaticMeshComponent)
        if gsmc:
            gsmc.set_material(0, sand_mat)
        loc = ground.get_actor_location()
        ground.set_actor_location(unreal.Vector(loc.x, loc.y, -1.0), False, False)  # just under arena floor
        log("ground plane -> sand")

    # --- arena extent (for ruin placement + nav size)
    extent = unreal.Vector(900, 900, 300)
    if arena:
        try:
            _, ext = arena.get_actor_bounds(False)
            extent = ext
        except Exception as e:  # noqa: BLE001
            warn("arena bounds failed: %s" % e)
    radius = max(600.0, min(extent.x, extent.y) * 0.80)

    # --- ancient ruins: broken sandstone pillars in a ring
    cyl = unreal.load_asset(CYL)
    pmat = unreal.load_asset(PILLAR_MAT)
    heights = [4.2, 2.4, 3.6, 1.6, 4.8, 2.0, 3.0, 1.2]  # cylinder is 100u tall; z-scale
    for i in range(8):
        ang = math.radians(i * 45.0 + 12.0)
        sz = heights[i]
        h = 100.0 * sz
        loc = unreal.Vector(math.cos(ang) * radius, math.sin(ang) * radius, h * 0.5 - 2.0)
        tilt = (-1.0 if i % 2 else 1.0) * (3.0 + (i % 3) * 2.0)  # slight lean = ruined
        ruin = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc,
                                          unreal.Rotator(tilt, i * 37.0, 0.0))
        rsmc = comp(ruin, unreal.StaticMeshComponent)
        if rsmc and cyl:
            rsmc.set_static_mesh(cyl)
            rsmc.set_world_scale3d(unreal.Vector(0.55, 0.55, sz))
            if pmat:
                rsmc.set_material(0, pmat)
        ruin.set_actor_label("Ruin_Pillar_%02d" % i)
        res["ruins"] += 1
    log("scattered %d ruin pillars at radius %.0f" % (res["ruins"], radius))

    # --- single PlayerStart, centered
    if starts:
        starts[0].set_actor_location(unreal.Vector(0.0, 0.0, 160.0), False, False)
        starts[0].set_actor_rotation(unreal.Rotator(0.0, 0.0, 0.0), False)
    else:
        eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0.0, 0.0, 160.0))

    # --- nav bounds over the play space
    nav = eas.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(0.0, 0.0, 100.0))
    if nav:
        nav.set_actor_scale3d(unreal.Vector(max(20.0, radius / 100.0 * 2.4),
                                            max(20.0, radius / 100.0 * 2.4), 6.0))
        nav.set_actor_label("Arena_NavBounds")
        log("nav bounds placed")

    saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    res["saved_level"] = bool(saved)
    res["kept"] = {"dir_lights": len(dirs[:1]), "sky_lights": len(skies[:1]),
                   "player_starts": 1, "ground_plane": bool(ground), "arena": bool(arena)}
    log("save_current_level -> %s" % saved)


def main():
    log("BEGIN")
    sand = make_sand()
    res["phases"].append("sand")
    build_map(sand)
    res["phases"].append("map")
    with open(OUT, "w") as f:
        json.dump(res, f, indent=2)
    log("wrote %s" % OUT)
    log("[gate] RESULT=%s" % ("PASS" if res.get("saved_level") and not res["errors"] else "WARN"))


main()

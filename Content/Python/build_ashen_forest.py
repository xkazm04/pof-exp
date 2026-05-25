"""
build_ashen_forest.py
======================
Authors the catalog-pipeline target zone **Ashen Forest** as a self-contained
greybox `.umap`: a scorched-woods clearing with charred-trunk scatter, ember
bonfire, POI markers, an ashen smoke atmosphere, and a baked-in functional test.

This is the `author-python` + `verify` recipe (src/lib/catalog/recipe.ts
ZONE_MAP_RECIPE) for the zone-map entity `z-ashen`, driven end-to-end.

Design choices (each is a deliberate, documented pipeline decision):
  * MOVABLE lights, never STATIONARY/STATIC + bake. Headless cooks skip the
    Lightmass bake, which leaves static-lit geometry pitch black (folder-05
    lesson). Movable lighting renders correctly with no bake step.
  * Greybox geometry from /Engine/BasicShapes (no FBX import) so the script has
    no Blender/Interchange dependency and runs fast. The ashen MOOD comes from
    the fog + post-process tint + dim ember lighting, not from textures.
  * The test gate REUSES the already-compiled `AVSArenaSetupTest` class (the
    zone-map recipe's per-section gate). Reusing a compiled UCLASS means NO C++
    recompile of the shared module — only a new, isolated .umap is written.
  * A brand-new map (/Game/Maps/AshenForest) — it never touches the shared
    VerticalSlice.umap, so it is safe under concurrent sessions on the shared
    UE tree.

NOTE on the map name: the recipe interpolates `/Game/Maps/<entity.data.id>.umap`,
which for id `z-ashen` would be the hyphenated `z-ashen.umap`. This script
sanitizes that to the PascalCase `AshenForest` (recommended recipe fix recorded
in the brief findings).

Run via the FULL editor (level Python needs Slate), headless:
    UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript="<abs path to this file>" ^
        -unattended -nopause -nosplash -abslog="<unique log path>"
The headless editor exits non-zero on a benign shutdown crash — judge success by
the `[build_ashen]` log lines, not the exit code.
"""
import math
import random
import unreal

LEVEL_PATH = "/Game/Maps/AshenForest"
SEED = 7321  # deterministic scatter

CUBE = "/Engine/BasicShapes/Cube"          # 100uu cube
CYL = "/Engine/BasicShapes/Cylinder"        # 100uu tall x 100uu dia, centered

asset_lib = unreal.EditorAssetLibrary


def _log(m):
    unreal.log("[build_ashen] " + m)


def _load(path):
    return asset_lib.load_asset(path) if asset_lib.does_asset_exist(path) else None


def _spawn_mesh(aes, mesh, loc, scale, label, mobility=unreal.ComponentMobility.STATIC):
    a = aes.spawn_actor_from_object(mesh, loc)
    a.set_actor_label(label)
    a.set_actor_scale3d(scale)
    smc = a.static_mesh_component
    if smc is not None:
        smc.set_mobility(mobility)
        smc.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    return a


def add_atmosphere(aes):
    """Unbound PostProcessVolume + ExponentialHeightFog tuned for ashen smoke.

    The PPV satisfies the gate's PostProcessVolume>=1 assertion AND gives the
    scorched-forest read: desaturated, warm-tinted, heavy haze.
    """
    # Idempotent: clear any prior atmosphere actors.
    for a in aes.get_all_level_actors():
        if isinstance(a, (unreal.PostProcessVolume, unreal.ExponentialHeightFog)):
            aes.destroy_actor(a)

    ppv = aes.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    ppv.set_actor_label("AshenForest_PostProcess")
    ppv.set_editor_property("unbound", True)
    s = ppv.get_editor_property("settings")
    # Desaturate + warm shift: ash greys with a dull ember undertone.
    s.set_editor_property("color_saturation", unreal.Vector4(0.7, 0.7, 0.7, 1.0))
    s.set_editor_property("override_color_saturation", True)
    try:  # color_gain name varies across versions — cosmetic, never abort on it
        s.set_editor_property("color_gain", unreal.Vector4(1.05, 0.95, 0.8, 1.0))
        s.set_editor_property("override_color_gain", True)
    except Exception:
        pass
    s.set_editor_property("vignette_intensity", 0.5)
    s.set_editor_property("override_vignette_intensity", True)
    s.set_editor_property("auto_exposure_bias", -0.5)
    s.set_editor_property("override_auto_exposure_bias", True)
    s.set_editor_property("bloom_intensity", 0.7)
    s.set_editor_property("override_bloom_intensity", True)
    ppv.set_editor_property("settings", s)

    fog = aes.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    fog.set_actor_label("AshenForest_Smoke")
    fc = fog.get_editor_property("component")
    fc.set_editor_property("fog_density", 0.45)
    fc.set_editor_property("fog_height_falloff", 0.12)
    tint = unreal.LinearColor(0.42, 0.34, 0.28, 1.0)  # warm grey smoke
    for prop in ("fog_inscattering_luminance", "fog_inscattering_color"):
        try:
            fc.set_editor_property(prop, tint)
            break
        except Exception:
            continue
    _log("Atmosphere: PostProcessVolume + ExponentialHeightFog (ashen smoke)")


def build():
    _log("=== Ashen Forest build START ===")
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    aes = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # Proven materialize -> bind -> spawn pattern (build_procgen_dungeon).
    les.new_level(LEVEL_PATH)
    les.save_current_level()
    les.load_level(LEVEL_PATH)
    _log("Created + bound level: " + LEVEL_PATH)

    # Idempotent reset of anything a prior run left behind.
    for a in aes.get_all_level_actors():
        if isinstance(a, (unreal.StaticMeshActor, unreal.DirectionalLight,
                          unreal.SkyLight, unreal.PointLight, unreal.PlayerStart,
                          unreal.VSArenaSetupTest)):
            aes.destroy_actor(a)

    cube = _load(CUBE)
    cyl = _load(CYL)
    if cube is None or cyl is None:
        raise RuntimeError("BasicShapes missing: cube=%s cyl=%s" % (cube, cyl))

    # --- Lighting: dim dusk through smoke (MOVABLE -> no bake needed) ---------
    dl = aes.spawn_actor_from_class(unreal.DirectionalLight,
                                    unreal.Vector(0, 0, 1200),
                                    unreal.Rotator(0.0, -18.0, -40.0))  # low sun
    for c in dl.get_components_by_class(unreal.DirectionalLightComponent):
        c.set_mobility(unreal.ComponentMobility.MOVABLE)
        c.set_editor_property("intensity", 2.5)  # overcast/smoke-dimmed
        try:
            c.set_editor_property("light_color", unreal.Color(255, 180, 130, 255))
        except Exception:
            pass
    dl.set_actor_label("AshenForest_Sun")

    sl = aes.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1200))
    for c in sl.get_components_by_class(unreal.SkyLightComponent):
        c.set_mobility(unreal.ComponentMobility.MOVABLE)
        c.set_editor_property("intensity", 1.2)
        try:
            c.set_editor_property("real_time_capture", True)
        except Exception:
            pass
    sl.set_actor_label("AshenForest_Sky")

    # --- Player spawn (via project BP_VSGameMode at PlayerStart) --------------
    aes.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 130))

    # --- Floor: a 44m x 44m scorched clearing --------------------------------
    _spawn_mesh(aes, cube, unreal.Vector(0, 0, -10), unreal.Vector(44, 44, 0.2),
                "AshenForest_Floor")
    _log("Floor placed (44x44m, top at z=0)")

    # --- Charred-trunk scatter (seeded, deterministic) -----------------------
    # Ring field 650..2000uu from centre; leaves a walkable central clearing +
    # spawn area clear. Each trunk is a thin tall cylinder (charred deadfall).
    rng = random.Random(SEED)
    trunks = 0
    placed = []
    attempts = 0
    while trunks < 26 and attempts < 400:
        attempts += 1
        ang = rng.uniform(0, 2 * math.pi)
        rad = rng.uniform(650, 2000)
        x = math.cos(ang) * rad
        y = math.sin(ang) * rad
        # spacing: reject if too close to an existing trunk
        if any((x - px) ** 2 + (y - py) ** 2 < 230 ** 2 for px, py in placed):
            continue
        placed.append((x, y))
        h = rng.uniform(3.2, 5.5)             # trunk height scale (cyl=100uu)
        r = rng.uniform(0.28, 0.42)           # trunk radius scale
        lean = rng.uniform(-8, 8)             # a few leaning, burnt-out trunks
        a = _spawn_mesh(aes, cyl,
                        unreal.Vector(x, y, h * 50.0),  # base sits on floor
                        unreal.Vector(r, r, h),
                        "AshenForest_Trunk_%02d" % trunks)
        a.set_actor_rotation(unreal.Rotator(lean, 0.0, rng.uniform(0, 360)), False)
        trunks += 1
    _log("Scattered %d charred trunks (seed %d)" % (trunks, SEED))

    # --- POIs: ember bonfire (PointLight + stump) + shrine + treasure --------
    # Bonfire: a movable warm point light = the discovery beacon / ambient glow.
    bonfire = aes.spawn_actor_from_class(unreal.PointLight, unreal.Vector(420, 260, 90))
    for c in bonfire.get_components_by_class(unreal.PointLightComponent):
        c.set_mobility(unreal.ComponentMobility.MOVABLE)
        c.set_editor_property("intensity", 6000.0)
        c.set_editor_property("attenuation_radius", 1400.0)
        try:
            c.set_editor_property("light_color", unreal.Color(255, 140, 60, 255))
        except Exception:
            pass
    bonfire.set_actor_label("AshenForest_POI_Bonfire")
    _spawn_mesh(aes, cyl, unreal.Vector(420, 260, 30), unreal.Vector(0.8, 0.8, 0.6),
                "AshenForest_POI_BonfireStump")
    _spawn_mesh(aes, cube, unreal.Vector(-900, 700, 60), unreal.Vector(1.2, 1.2, 2.4),
                "AshenForest_POI_Shrine")
    _spawn_mesh(aes, cube, unreal.Vector(-650, -850, 40), unreal.Vector(0.8, 0.8, 0.8),
                "AshenForest_POI_Treasure")
    _log("Placed POIs: Bonfire (ember light), Shrine, Treasure")

    add_atmosphere(aes)

    # --- Bake the functional test gate into the map --------------------------
    # Reuse the compiled AVSArenaSetupTest (lighting/PP setup invariant). The
    # automation test name is taken from the actor label.
    gate = aes.spawn_actor_from_class(unreal.VSArenaSetupTest, unreal.Vector(0, 0, 300))
    gate.set_actor_label("AshenForestSetupTest")
    _log("Baked gate actor AshenForestSetupTest (AVSArenaSetupTest)")

    les.save_current_level()
    _log("Saved level: " + LEVEL_PATH)

    # --- Persistence check: reload from disk and recount ---------------------
    les.load_level(LEVEL_PATH)
    a2 = aes.get_all_level_actors()
    dirl = len([a for a in a2 if isinstance(a, unreal.DirectionalLight)])
    skyl = len([a for a in a2 if isinstance(a, unreal.SkyLight)])
    ppv = len([a for a in a2 if isinstance(a, unreal.PostProcessVolume)])
    sma = len([a for a in a2 if isinstance(a, unreal.StaticMeshActor)])
    tests = len([a for a in a2 if isinstance(a, unreal.VSArenaSetupTest)])
    _log("Persisted after reload: DirLight=%d SkyLight=%d PPV=%d StaticMeshes=%d Tests=%d"
         % (dirl, skyl, ppv, sma, tests))
    ok = dirl >= 1 and skyl >= 1 and ppv >= 1 and tests >= 1
    _log("GATE PRECONDITIONS %s" % ("OK" if ok else "MISSING"))
    _log("=== Ashen Forest build COMPLETE ===")


def main():
    try:
        build()
    except Exception as exc:
        unreal.log_error("[build_ashen] BUILD FAILED: " + str(exc))
        raise


if __name__ == "__main__":
    main()
    # Full editor (-ExecutePythonScript) has no commandlet to end the process;
    # quit once the work is done.
    try:
        if unreal.is_editor():
            unreal.SystemLibrary.quit_editor()
    except Exception:
        pass

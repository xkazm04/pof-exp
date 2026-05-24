"""
scatter_biome_ue.py
===================
Author a BiomeDefinition (placeholder engine meshes) + place an
AARPGVegetationScatter over the VerticalSlice arena floor and generate
no-collision greybox props. Env params: SCATTER_DENSITY, SCATTER_SEED.

Run via the FULL editor:
    UnrealEditor.exe <uproject> -ExecutePythonScript="<abs path>" -unattended -nopause -nosplash
"""
import os
import unreal

BIOME_PATH = "/Game/Level/Biomes/BD_ArenaRubble"
LEVEL_PATH = "/Game/Maps/VerticalSlice"
DENSITY = float(os.environ.get("SCATTER_DENSITY", "1.0"))
SEED = int(os.environ.get("SCATTER_SEED", "1337"))

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_lib = unreal.EditorAssetLibrary


def _log(m):
    unreal.log("[scatter_biome] " + m)


def make_biome():
    folder = "/Game/Level/Biomes"
    if asset_lib.does_asset_exist(BIOME_PATH):
        biome = asset_lib.load_asset(BIOME_PATH)
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.ARPGBiomeDefinition)
        biome = asset_tools.create_asset("BD_ArenaRubble", folder, unreal.ARPGBiomeDefinition, factory)
    if biome is None:
        raise RuntimeError("failed to create/load biome " + BIOME_PATH)

    cube = asset_lib.load_asset("/Engine/BasicShapes/Cube")
    cyl = asset_lib.load_asset("/Engine/BasicShapes/Cylinder")

    layer = unreal.BiomeScatterLayer()
    layer.set_editor_property("layer_name", unreal.Name("rubble"))
    layer.set_editor_property("meshes", [cube, cyl])
    layer.set_editor_property("density_per100_sq", 0.15)
    layer.set_editor_property("min_scale", 0.3)
    layer.set_editor_property("max_scale", 0.7)
    layer.set_editor_property("min_slope_angle", 0.0)
    layer.set_editor_property("max_slope_angle", 60.0)
    layer.set_editor_property("min_spacing", 150.0)
    layer.set_editor_property("align_to_surface", True)

    biome.set_editor_property("biome_id", unreal.Name("ArenaRubble"))
    biome.set_editor_property("scatter_layers", [layer])
    biome.set_editor_property("global_density_multiplier", 1.0)
    asset_lib.save_asset(BIOME_PATH)
    n = len(biome.get_editor_property("scatter_layers"))
    _log("Biome BD_ArenaRubble: %d scatter layer(s)" % n)
    return biome


def main():
    _log("=== Biome scatter START (density=%.2f seed=%d) ===" % (DENSITY, SEED))
    biome = make_biome()

    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.load_level(LEVEL_PATH)
    aes = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # Idempotent: remove any prior scatter actor.
    for a in aes.get_all_level_actors():
        if isinstance(a, unreal.ARPGVegetationScatter):
            aes.destroy_actor(a)

    scatter = aes.spawn_actor_from_class(unreal.ARPGVegetationScatter, unreal.Vector(0.0, 0.0, 200.0))
    scatter.set_actor_label("Arena_Scatter")
    scatter.set_editor_property("biome_definition", biome)
    scatter.set_editor_property("random_seed", SEED)
    scatter.set_editor_property("local_density_multiplier", DENSITY)
    # Do NOT regenerate at runtime — that would rebuild the HISM instances with
    # default collision and discard the edit-time NO_COLLISION set below. Keep
    # the baked (no-collision) instances so the player passes through.
    scatter.set_editor_property("generate_on_begin_play", False)
    # Bounds box spans above + through the floor for downward visibility traces.
    bounds = scatter.get_editor_property("scatter_bounds")
    bounds.set_box_extent(unreal.Vector(1000.0, 1000.0, 200.0))

    scatter.generate_vegetation()
    count = scatter.get_total_instance_count()
    _log("Scattered %d instances" % count)
    if count <= 0:
        unreal.log_warning("[scatter_biome] 0 instances — check bounds/trace/floor")

    # Force no-collision so the player passes through (VSFunctionalTest safe).
    for comp in scatter.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent):
        comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    _log("Scatter HISM set to NO_COLLISION")

    les.save_current_level()
    les.load_level(LEVEL_PATH)
    persisted = sum(1 for a in aes.get_all_level_actors() if isinstance(a, unreal.ARPGVegetationScatter))
    _log("Persisted after reload: scatter actors=%d" % persisted)
    _log("=== Biome scatter COMPLETE ===")


if __name__ == "__main__":
    main()
    try:
        if unreal.is_editor():
            unreal.SystemLibrary.quit_editor()
    except Exception:
        pass

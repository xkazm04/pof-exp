"""
build_test_inventory_map.py
===========================
Stream 4 (Inventory) test map. Produces /Game/Maps/Test_Inventory:

  - A duplicate of the lit Arena_Ancient (known-good lighting + the GAS player
    pawn spawn), with enemies removed (the inventory proof isn't a combat test).
  - One AARPGLootChest placed ~250u in front of the PlayerStart, its LootTable set
    to LT_ChestPotion (drops the health potion 100%), ItemScatterRadius small so the
    dropped potion lands close to the chest (well within the scenario's collect sweep).

The scenario (shots/inventory/inv-loot-heal.json) opens this chest, collects the
potion into the player's inventory, sets start health to 50, and uses the potion.

Idempotent: re-running rebuilds the map. Run headless (commandlet, clean exit):
    "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
        "C:\\Users\\kazda\\Documents\\Unreal Projects\\PoF-inventory\\PoF.uproject" ^
        -run=pythonscript -script="<abs path to this file>" -unattended -nopause
"""

import math

import unreal

SRC_MAP = "/Game/Maps/Arena_Ancient"
DST_MAP = "/Game/Maps/Test_Inventory"
LOOT_TABLE_PATH = "/Game/Inventory/LT_ChestPotion"
CHEST_CLASS_PATH = "/Script/PoF.ARPGLootChest"
ENEMY_CLASS_PATH = "/Script/PoF.ARPGEnemyCharacter"
CUBE_MESH = "/Engine/BasicShapes/Cube"
CHEST_FORWARD_DIST = 250.0

asset_lib = unreal.EditorAssetLibrary
_PROGRESS = []


def _log(msg):
    unreal.log_warning("[build_inv_map] " + msg)
    _PROGRESS.append(msg)


def load_class(class_path):
    cls = unreal.load_class(None, class_path)
    if cls is None:
        cls = unreal.load_class(None, class_path + "_C")
    return cls


def build():
    level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if not asset_lib.does_asset_exist(SRC_MAP):
        raise RuntimeError("Source map missing: " + SRC_MAP)

    # Clean rebuild: move off the target, delete it, re-duplicate from the lit source.
    if asset_lib.does_asset_exist(DST_MAP):
        level_sub.new_level("/Temp/_inv_scratch")
        asset_lib.delete_asset(DST_MAP)
        _log("Deleted prior " + DST_MAP)
    if not asset_lib.duplicate_asset(SRC_MAP, DST_MAP):
        raise RuntimeError("duplicate_asset failed: %s -> %s" % (SRC_MAP, DST_MAP))
    _log("Duplicated %s -> %s" % (SRC_MAP, DST_MAP))

    if not level_sub.load_level(DST_MAP):
        raise RuntimeError("load_level failed: " + DST_MAP)

    all_actors = actor_sub.get_all_level_actors()

    # Remove enemies — this is an inventory test, not a combat test. (Best-effort; the
    # scenario's disable_ai removes any AI-possessed pawn at runtime regardless.)
    enemy_py = getattr(unreal, "ARPGEnemyCharacter", None)
    removed = 0
    if enemy_py is not None:
        for a in all_actors:
            if isinstance(a, enemy_py):
                actor_sub.destroy_actor(a)
                removed += 1
    _log("Removed %d enemy actor(s)" % removed)

    # Find the PlayerStart to anchor the chest in front of the player.
    player_start = None
    for a in all_actors:
        if isinstance(a, unreal.PlayerStart):
            player_start = a
            break
    if player_start is None:
        raise RuntimeError("No PlayerStart in " + DST_MAP)
    ps_loc = player_start.get_actor_location()
    ps_rot = player_start.get_actor_rotation()
    yaw_rad = math.radians(ps_rot.yaw)
    fwd = unreal.Vector(math.cos(yaw_rad), math.sin(yaw_rad), 0.0)
    chest_loc = unreal.Vector(
        ps_loc.x + fwd.x * CHEST_FORWARD_DIST,
        ps_loc.y + fwd.y * CHEST_FORWARD_DIST,
        ps_loc.z)
    # Chest faces back toward the player (so its forward-biased loot drops between them).
    chest_yaw = ps_rot.yaw + 180.0
    chest_rot = unreal.Rotator(0.0, 0.0, chest_yaw)

    chest_cls = load_class(CHEST_CLASS_PATH)
    if chest_cls is None:
        raise RuntimeError("Could not load " + CHEST_CLASS_PATH + " (rebuild the editor?)")
    chest = actor_sub.spawn_actor_from_class(chest_cls, chest_loc, chest_rot)
    if chest is None:
        raise RuntimeError("Failed to spawn chest")
    chest.set_actor_label("PotionChest")

    loot_table = asset_lib.load_asset(LOOT_TABLE_PATH)
    if loot_table is None:
        raise RuntimeError("Loot table missing: " + LOOT_TABLE_PATH + " (run author_inventory_assets.py first)")
    chest.set_editor_property("loot_table", loot_table)
    chest.set_editor_property("num_rolls", 1)
    chest.set_editor_property("item_scatter_radius", 0.0)

    # Make the chest visible (its mesh comps have no default mesh).
    cube = asset_lib.load_asset(CUBE_MESH)
    if cube is not None:
        try:
            base = chest.get_editor_property("base_mesh")
            lid = chest.get_editor_property("lid_mesh")
            if base:
                base.set_editor_property("static_mesh", cube)
                base.set_relative_scale3d(unreal.Vector(1.0, 1.2, 0.6))
            if lid:
                lid.set_editor_property("static_mesh", cube)
                lid.set_relative_scale3d(unreal.Vector(1.0, 1.2, 0.2))
                lid.set_relative_location(unreal.Vector(0.0, 0.0, 40.0))
        except Exception as exc:  # noqa: BLE001
            _log("WARN: could not assign chest meshes: " + str(exc))

    _log("Placed PotionChest at (%.0f,%.0f,%.0f) yaw=%.0f, loot=LT_ChestPotion" % (
        chest_loc.x, chest_loc.y, chest_loc.z, chest_yaw))

    level_sub.save_current_level()
    _log("Saved " + DST_MAP)


def main():
    _log("=== build_test_inventory_map START ===")
    build()
    _log("=== build_test_inventory_map COMPLETE ===")
    try:
        out_path = unreal.Paths.combine(
            [unreal.Paths.project_saved_dir(), "build_test_inventory_map.log"])
        with open(out_path, "w", encoding="utf-8") as fh:
            fh.write("\n".join(_PROGRESS) + "\n")
        unreal.log_warning("[build_inv_map] progress written to " + out_path)
    except Exception as exc:  # noqa: BLE001
        unreal.log_error("[build_inv_map] could not write progress log: " + str(exc))


if __name__ == "__main__":
    main()

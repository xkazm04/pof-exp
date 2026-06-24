"""
author_inventory_assets.py
===========================
Stream 4 (Inventory) content authoring. Creates, in /Game/Inventory/:

  - DA_HealthPotion   a UARPGItemDefinition consumable. Type=Consumable,
                      OnUseEffect=UGE_HealthPotion (+50 instant heal), a visible
                      WorldMesh so the dropped item shows, MaxStackSize=10.
                      DisplayName "Minor Health Potion" matches the app's seeded
                      item-7 (schema-down lockstep).
  - LT_ChestPotion    a UARPGLootTable that drops DA_HealthPotion 100% (one entry,
                      NothingWeight=0). The Test_Inventory chest uses this.

Idempotent: re-running updates the assets in place.

Run via the FULL editor (so the asset registry + GE classes are loaded), -nullrhi:
    "C:\\Program Files\\Epic Games\\UE_5.8\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
        "C:\\Users\\kazda\\Documents\\Unreal Projects\\PoF-inventory\\PoF.uproject" ^
        -ExecutePythonScript="<abs path to this file>" -unattended -nopause -nullrhi
"""

import unreal

INV_DIR = "/Game/Inventory"
ITEM_DEF_CLASS_PATH = "/Script/PoF.ARPGItemDefinition"
LOOT_TABLE_CLASS_PATH = "/Script/PoF.ARPGLootTable"
HEAL_GE_CLASS_PATH = "/Script/PoF.GE_HealthPotion"
POTION_MESH = "/Engine/BasicShapes/Sphere"

POTION_PATH = INV_DIR + "/DA_HealthPotion"
LOOT_PATH = INV_DIR + "/LT_ChestPotion"

asset_lib = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
_PROGRESS = []


def _log(msg):
    unreal.log_warning("[author_inventory] " + msg)
    _PROGRESS.append(msg)


def _get_or_create(path, name, cls):
    """Idempotent: update in place if it exists, else create via DataAssetFactory."""
    if asset_lib.does_asset_exist(path):
        asset = asset_lib.load_asset(path)
        _log("Loaded existing %s for in-place update" % path)
        return asset
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    asset = asset_tools.create_asset(name, INV_DIR, cls, factory)
    _log("Created new %s" % path)
    return asset


def author_potion():
    _log("--- Authoring %s ---" % POTION_PATH)
    item_cls = unreal.load_class(None, ITEM_DEF_CLASS_PATH)
    if item_cls is None:
        raise RuntimeError("Could not load " + ITEM_DEF_CLASS_PATH + " (rebuild the editor?)")

    asset = _get_or_create(POTION_PATH, "DA_HealthPotion", item_cls)
    if asset is None:
        raise RuntimeError("could not load or create " + POTION_PATH)

    asset.set_editor_property("display_name", "Minor Health Potion")
    asset.set_editor_property("description", "Restores 50 health when consumed.")
    asset.set_editor_property("type", unreal.ARPGItemType.CONSUMABLE)
    asset.set_editor_property("rarity", unreal.ARPGItemRarity.COMMON)
    asset.set_editor_property("max_stack_size", 10)
    asset.set_editor_property("base_value", 25.0)
    asset.set_editor_property("weight", 0.5)
    asset.set_editor_property("required_level", 0)
    asset.set_editor_property("allowed_slots", [])  # consumable: not equippable

    # OnUseEffect (TSubclassOf<UGameplayEffect>) — the +50 heal applied by UseItem.
    ge_class = unreal.load_class(None, HEAL_GE_CLASS_PATH)
    if ge_class is None:
        raise RuntimeError("Could not load heal GE " + HEAL_GE_CLASS_PATH)
    asset.set_editor_property("on_use_effect", ge_class)

    # WorldMesh — visible mesh so the dropped potion renders in the world.
    mesh = asset_lib.load_asset(POTION_MESH)
    if mesh is not None:
        asset.set_editor_property("world_mesh", mesh)
    else:
        _log("WARN: world mesh missing: " + POTION_MESH)

    # ItemTags — FGameplayTag exposes only TagName to Python; build the struct directly.
    try:
        tag = unreal.GameplayTag()
        tag.set_editor_property("tag_name", unreal.Name("Item.Consumable.Potion"))
        container = unreal.GameplayTagContainer()
        container.set_editor_property("gameplay_tags", [tag])
        asset.set_editor_property("item_tags", container)
    except Exception as exc:  # noqa: BLE001
        _log("WARN: could not set ItemTags: " + str(exc))

    asset_lib.save_loaded_asset(asset)

    rb = asset_lib.load_asset(POTION_PATH)
    _log("VERIFY potion DisplayName=%s Type=%s OnUse=%s WorldMesh=%s MaxStack=%d" % (
        str(rb.get_editor_property("display_name")),
        str(rb.get_editor_property("type")),
        "set" if rb.get_editor_property("on_use_effect") else "NULL",
        "set" if rb.get_editor_property("world_mesh") else "NULL",
        rb.get_editor_property("max_stack_size"),
    ))
    return asset


def author_loot_table(potion_asset):
    _log("--- Authoring %s ---" % LOOT_PATH)
    loot_cls = unreal.load_class(None, LOOT_TABLE_CLASS_PATH)
    if loot_cls is None:
        raise RuntimeError("Could not load " + LOOT_TABLE_CLASS_PATH + " (rebuild the editor?)")

    table = _get_or_create(LOOT_PATH, "LT_ChestPotion", loot_cls)
    if table is None:
        raise RuntimeError("could not load or create " + LOOT_PATH)

    entry = unreal.LootEntry()
    entry.set_editor_property("item", potion_asset)
    entry.set_editor_property("drop_weight", 1.0)
    entry.set_editor_property("min_quantity", 1)
    entry.set_editor_property("max_quantity", 1)
    entry.set_editor_property("min_rarity", unreal.ARPGItemRarity.COMMON)
    entry.set_editor_property("max_rarity", unreal.ARPGItemRarity.COMMON)

    table.set_editor_property("entries", [entry])
    table.set_editor_property("nothing_weight", 0.0)
    asset_lib.save_loaded_asset(table)

    rb = asset_lib.load_asset(LOOT_PATH)
    entries = rb.get_editor_property("entries")
    item0 = entries[0].get_editor_property("item") if len(entries) else None
    _log("VERIFY loot Entries=%d Entry0.Item=%s NothingWeight=%.1f" % (
        len(entries),
        item0.get_name() if item0 else "NULL",
        rb.get_editor_property("nothing_weight"),
    ))


def main():
    _log("=== author_inventory_assets START ===")
    if not asset_lib.does_directory_exist(INV_DIR):
        asset_lib.make_directory(INV_DIR)
        _log("Created directory " + INV_DIR)
    potion = author_potion()
    author_loot_table(potion)
    _log("=== author_inventory_assets COMPLETE ===")
    try:
        out_path = unreal.Paths.combine(
            [unreal.Paths.project_saved_dir(), "author_inventory_assets.log"])
        with open(out_path, "w", encoding="utf-8") as fh:
            fh.write("\n".join(_PROGRESS) + "\n")
        unreal.log_warning("[author_inventory] progress written to " + out_path)
    except Exception as exc:  # noqa: BLE001
        unreal.log_error("[author_inventory] could not write progress log: " + str(exc))


if __name__ == "__main__":
    main()

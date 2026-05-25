"""
author_items.py
================
Authors real UARPGItemDefinition data assets for the `items` catalog from the
PoF app's canonical entity data (Catalog Pipeline, Core/Existing -> items row).

Lead target: the Iron Longsword (catalog entity `item-1`) ->
    /Game/Data/Items/DA_IronLongsword

Each entry maps a catalog ItemData record onto the UARPGItemDefinition schema.
The weapon's offense is carried by OnEquipEffect (a GameplayEffect class), since
UARPGItemDefinition has no intrinsic damage field — the equip GE grants the
attribute bonus. For the Iron Longsword that is UGE_Equip_IronLongsword
(+15 AttackPower = the canonical avg of its 12-18 damage).

Idempotent: re-running overwrites the asset. Run via the FULL editor
(-ExecutePythonScript, not -run=pythonscript) so the asset registry + GE classes
are fully loaded:

    "C:\\Program Files\\Epic Games\\UE_5.7\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
        "C:\\Users\\kazda\\Documents\\Unreal Projects\\PoF\\PoF.uproject" ^
        -ExecutePythonScript="<abs path to this file>" -unattended -nopause -nullrhi
"""

import unreal

ITEMS_DIR = "/Game/Data/Items"
ITEM_DEF_CLASS_PATH = "/Script/PoF.ARPGItemDefinition"

asset_lib = unreal.EditorAssetLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
_PROGRESS = []


def _log(msg):
    unreal.log_warning("[author_items] " + msg)
    _PROGRESS.append(msg)


# ── Catalog entities -> UARPGItemDefinition field mapping ────────────────────
# Mirrors src/lib/catalog/seed-items.ts -> DUMMY_ITEMS[item-1].
IRON_LONGSWORD = {
    "asset_name": "DA_IronLongsword",
    "display_name": "Iron Longsword",
    "description": "A standard issue longsword.",
    "type": unreal.ARPGItemType.WEAPON,
    "rarity": unreal.ARPGItemRarity.COMMON,
    "max_stack_size": 1,
    "base_value": 12.0,          # modest common-tier gold value
    "weight": 3.5,               # typical longsword encumbrance
    "required_level": 1,         # common starter weapon
    "allowed_slots": [unreal.EquipmentSlot.WEAPON],
    "item_tag": "Item.Weapon.Sword",
    "on_equip_effect_class": "/Script/PoF.GE_Equip_IronLongsword",
}


def author_item(spec):
    name = spec["asset_name"]
    package_path = ITEMS_DIR + "/" + name
    _log("--- Authoring %s ---" % package_path)

    item_def_class = unreal.load_class(None, ITEM_DEF_CLASS_PATH)
    if item_def_class is None:
        raise RuntimeError("Could not load " + ITEM_DEF_CLASS_PATH + " (rebuild the editor?)")

    # Idempotent: update in place if it exists, else create. (delete_asset +
    # create_asset is NOT idempotent — create won't recreate at a just-deleted
    # path in the same session.)
    if asset_lib.does_asset_exist(package_path):
        asset = asset_lib.load_asset(package_path)
        _log("Loaded existing asset for in-place update")
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", item_def_class)
        asset = asset_tools.create_asset(name, ITEMS_DIR, item_def_class, factory)
        _log("Created new asset")
    if asset is None:
        raise RuntimeError("could not load or create " + package_path)

    # Core schema fields (str auto-converts to FText for FText properties).
    asset.set_editor_property("display_name", spec["display_name"])
    asset.set_editor_property("description", spec["description"])
    asset.set_editor_property("type", spec["type"])
    asset.set_editor_property("rarity", spec["rarity"])
    asset.set_editor_property("max_stack_size", spec["max_stack_size"])
    asset.set_editor_property("base_value", spec["base_value"])
    asset.set_editor_property("weight", spec["weight"])
    asset.set_editor_property("required_level", spec["required_level"])
    asset.set_editor_property("allowed_slots", spec["allowed_slots"])

    # OnEquipEffect (TSubclassOf<UGameplayEffect>) — carries the weapon's offense.
    ge_class = unreal.load_class(None, spec["on_equip_effect_class"])
    if ge_class is None:
        raise RuntimeError("Could not load equip GE " + spec["on_equip_effect_class"])
    asset.set_editor_property("on_equip_effect", ge_class)

    # ItemTags (FGameplayTagContainer) — FGameplayTag exposes only TagName (FName)
    # to Python, so build the tag struct directly. Best effort; non-fatal.
    try:
        tag = unreal.GameplayTag()
        tag.set_editor_property("tag_name", unreal.Name(spec["item_tag"]))
        container = unreal.GameplayTagContainer()
        container.set_editor_property("gameplay_tags", [tag])
        asset.set_editor_property("item_tags", container)
        _log("Set ItemTags = " + spec["item_tag"])
    except Exception as exc:  # noqa: BLE001 — tag API churns across engine versions
        _log("WARN: could not set ItemTags (%s): %s" % (spec["item_tag"], str(exc)))

    saved = asset_lib.save_loaded_asset(asset)
    _log("Saved %s (save_loaded_asset=%s)" % (package_path, saved))

    # Read-back verification — logged so the headless run carries the evidence.
    # Defensive: a logging hiccup must never fail an already-saved asset.
    try:
        rb = asset_lib.load_asset(package_path)
        _log("VERIFY DisplayName=%s Type=%s Rarity=%s MaxStack=%d BaseValue=%.1f Slots=%d Tags=%d OnEquip=%s" % (
            str(rb.get_editor_property("display_name")),
            str(rb.get_editor_property("type")),
            str(rb.get_editor_property("rarity")),
            rb.get_editor_property("max_stack_size"),
            rb.get_editor_property("base_value"),
            len(rb.get_editor_property("allowed_slots")),
            len(rb.get_editor_property("item_tags").get_editor_property("gameplay_tags")),
            "set" if rb.get_editor_property("on_equip_effect") else "NULL",
        ))
    except Exception as exc:  # noqa: BLE001
        _log("WARN: read-back verification logging failed (asset is still saved): " + str(exc))


def main():
    _log("=== author_items START ===")
    if not asset_lib.does_directory_exist(ITEMS_DIR):
        asset_lib.make_directory(ITEMS_DIR)
        _log("Created directory " + ITEMS_DIR)
    author_item(IRON_LONGSWORD)
    _log("=== author_items COMPLETE ===")
    try:
        out_path = unreal.Paths.combine(
            [unreal.Paths.project_saved_dir(), "author_items.log"])
        with open(out_path, "w", encoding="utf-8") as fh:
            fh.write("\n".join(_PROGRESS) + "\n")
        unreal.log_warning("[author_items] progress written to " + out_path)
    except Exception as exc:  # noqa: BLE001
        unreal.log_error("[author_items] could not write progress log: " + str(exc))


if __name__ == "__main__":
    main()

"""
seed_vendor_inventory.py
========================
Writes /Game/Economy/Vendors/DT_VendorInventory (a UDataTable of
FARPGVendorInventoryRow) — one row per vendor, keyed by the vendor id. Catalog
pipeline: vendors -> UE Packaging (consumed by UARPGVendorComponent at runtime;
pricing goes through UARPGVendorRules / UARPGFactionRules).

Row values mirror the app-side contract in src/lib/catalog/pipelines/vendors.ts
(30% markup, 50% buyback, reputation discount by tier).

Run headless after the C++ types compile:
    "...UnrealEditor-Cmd.exe" "...PoF.uproject" -run=pythonscript -script="<abs>" -unattended -nopause -abslog="..."
"""

import unreal

PACKAGE_FOLDER = "/Game/Economy/Vendors"
ASSET_NAME = "DT_VendorInventory"
PACKAGE_PATH = f"{PACKAGE_FOLDER}/{ASSET_NAME}"
ROW_STRUCT_PATH = "/Script/PoF.ARPGVendorInventoryRow"

# One stock line per seeded vendor. Row name == the vendor id (PascalCase slug of the
# vendor name, e.g. "Wandering Merchant" -> "WanderingMerchant").
VENDOR_ROWS = {
    "WanderingMerchant": [
        # (ItemId, BaseCost, Stock) — BaseCost is the theoretical cost the 30% markup applies over.
        ("item-7", 20, -1),   # Minor Health Potion — unlimited
        ("item-1", 45, 3),    # Iron Longsword — limited stock
    ],
}


def build_table():
    row_struct = unreal.load_object(None, ROW_STRUCT_PATH)
    if row_struct is None:
        unreal.log_warning("[seed_vendor_inventory] FARPGVendorInventoryRow struct not found — compile C++ first.")
        return None

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    if unreal.EditorAssetLibrary.does_asset_exist(PACKAGE_PATH):
        table = unreal.EditorAssetLibrary.load_asset(PACKAGE_PATH)
    else:
        factory = unreal.DataTableFactory()
        factory.set_editor_property("struct", row_struct)
        table = asset_tools.create_asset(ASSET_NAME, PACKAGE_FOLDER, unreal.DataTable, factory)

    # Fill one row per vendor; the first stock line represents the vendor's headline stock.
    rows_json = {}
    for vendor_id, lines in VENDOR_ROWS.items():
        item_id, base_cost, stock = lines[0]
        rows_json[vendor_id] = {"ItemId": item_id, "BaseCost": base_cost, "Stock": stock}

    import json
    unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows_json))
    unreal.EditorAssetLibrary.save_asset(PACKAGE_PATH)
    unreal.log(f"[seed_vendor_inventory] seeded {len(rows_json)} vendor rows into {PACKAGE_PATH}")
    return table


if __name__ == "__main__":
    build_table()

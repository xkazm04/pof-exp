"""
seed_generated_abilities.py
===========================
Reads Source/PoF/AbilitySystem/Effects/Generated/manifest.json (emitted by the app's
"Generate C++" dispatch) and writes /Game/Abilities/Generated/DT_GeneratedAbilities
(a UDataTable of FARPGGeneratedAbilityRow) — the additive registry of generated abilities.

Population uses the editor JSON-import path (DataTableFunctionLibrary.fill_data_table_from_json_string);
UDataTableFunctionLibrary has no add/remove-row API in this engine version. Each manifest
entry becomes one row keyed by its name (fill upserts by name on re-runs). The class paths
are validated (loaded) first as a proof that the generated classes exist; the soft refs in
the table store the paths.

Run headless after the C++ types + generated classes compile:
    "...UnrealEditor-Cmd.exe" "...PoF.uproject" -run=pythonscript -script="<abs>" -unattended -nopause -abslog="..."
"""

import json
import os
import unreal

PACKAGE_FOLDER = "/Game/Abilities/Generated"
ASSET_NAME = "DT_GeneratedAbilities"
PACKAGE_PATH = f"{PACKAGE_FOLDER}/{ASSET_NAME}"
ROW_STRUCT_PATH = "/Script/PoF.ARPGGeneratedAbilityRow"

PROJECT_DIR = unreal.Paths.project_dir()
MANIFEST_PATH = os.path.join(
    PROJECT_DIR, "Source", "PoF", "AbilitySystem", "Effects", "Generated", "manifest.json"
)


def load_manifest():
    if not os.path.isfile(MANIFEST_PATH):
        unreal.log_warning(f"[seed_generated_abilities] No manifest at {MANIFEST_PATH} — seeding 0 rows.")
        return []
    with open(MANIFEST_PATH, "r", encoding="utf-8") as f:
        return json.load(f).get("abilities", [])


def ensure_folder(path):
    eal = unreal.EditorAssetLibrary
    if not eal.does_directory_exist(path):
        eal.make_directory(path)


def load_row_struct():
    s = unreal.load_object(None, ROW_STRUCT_PATH)
    if s is None:
        raise RuntimeError(f"Could not load {ROW_STRUCT_PATH}. Recompile after adding ARPGGeneratedAbilityTypes.h.")
    return s


def get_or_create_datatable(row_struct):
    eal = unreal.EditorAssetLibrary
    if eal.does_asset_exist(PACKAGE_PATH):
        unreal.log(f"[seed_generated_abilities] Reusing {PACKAGE_PATH} (fill upserts by row name).")
        return eal.load_asset(PACKAGE_PATH)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.DataTableFactory()
    factory.struct = row_struct
    dt = tools.create_asset(ASSET_NAME, PACKAGE_FOLDER, unreal.DataTable, factory)
    if dt is None:
        raise RuntimeError(f"Failed to create DataTable at {PACKAGE_PATH}")
    unreal.log(f"[seed_generated_abilities] Created {PACKAGE_PATH}.")
    return dt


def validate_class(path):
    """Load the class to PROVE the generated class exists. Logs (does not raise) on failure."""
    if not path:
        return
    cls = None
    try:
        cls = unreal.load_class(None, path)
    except Exception:
        cls = None
    if cls is None:
        try:
            cls = unreal.load_object(None, path)
        except Exception:
            cls = None
    if cls is None:
        unreal.log_warning(f"[seed_generated_abilities] Could not load class {path}.")


def build_rows(abilities):
    """One DataTable-JSON object per ability. 'Name' is the row key; soft-class fields
    take the /Script/... path strings. The gameplay tag is omitted (app-only tags are not
    registered natively, so they would not import)."""
    rows = []
    for e in abilities:
        ability_path = e.get("abilityClass", "")
        validate_class(ability_path)
        effect_paths = e.get("effectClasses", [])
        for p in effect_paths:
            validate_class(p)
        rows.append({
            "Name": e.get("name", ""),
            "AbilityClass": ability_path,
            "EffectClasses": effect_paths,
        })
    return rows


# Built-in status-effect abilities that are always seeded regardless of the manifest
# (catalog pipeline: status-effect -> UE Packaging). "Burning" is the fire DoT applied by
# GE_Gen_Burning (grants State.Burning, periodic execution); it has a live L3 gate
# (VSStatusBurningEffectTest). Kept here so the row exists even before the generator runs.
BUILTIN_ABILITIES = [
    {
        "name": "Burning",
        "abilityClass": "",
        "effectClasses": ["/Game/Abilities/Generated/GE_Gen_Burning"],
    },
]


def merge_builtins(abilities):
    """Append built-in abilities that the manifest did not already define (by name)."""
    have = {a.get("name", "") for a in abilities}
    return list(abilities) + [b for b in BUILTIN_ABILITIES if b["name"] not in have]


def main():
    unreal.log("[seed_generated_abilities] Starting.")
    abilities = merge_builtins(load_manifest())
    ensure_folder(PACKAGE_FOLDER)
    row_struct = load_row_struct()
    dt = get_or_create_datatable(row_struct)

    rows = build_rows(abilities)
    # fill_data_table_from_json_string returns True on success.
    ok = unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(dt, json.dumps(rows))
    if not ok:
        unreal.log_error("[seed_generated_abilities] fill_data_table_from_json_string reported failure.")

    unreal.EditorAssetLibrary.save_asset(PACKAGE_PATH)
    count = len(list(unreal.DataTableFunctionLibrary.get_data_table_row_names(dt)))
    unreal.log(f"[seed_generated_abilities] Saved {PACKAGE_PATH} with {count} rows.")


if __name__ == "__main__":
    main()

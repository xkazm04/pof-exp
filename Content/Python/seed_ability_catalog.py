"""
seed_ability_catalog.py
=======================
Creates (or rewrites) /Game/Abilities/DT_AbilityCatalog as a UDataTable using
FARPGAbilityCatalogRow and seeds it with the same rows the PoF app keeps in
sub_character/_shared/data.ts > CHARACTER_ABILITIES (Phase 2 F3).

The PoF app side is the SYNC SOURCE — re-run this script whenever the app-side
list changes so the two stay in lockstep.

Run headless after the C++ types compile:
    "C:\\Program Files\\Epic Games\\UE_5.7\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
        "C:\\Users\\kazda\\Documents\\Unreal Projects\\PoF\\PoF.uproject" ^
        -run=pythonscript -script="<abs path to this file>" ^
        -unattended -nopause ^
        -abslog="C:\\Users\\kazda\\Documents\\Unreal Projects\\PoF\\Saved\\Logs\\seed_ability_catalog.log"
"""

import unreal

# ---------------------------------------------------------------------------
# Catalog asset target
# ---------------------------------------------------------------------------

PACKAGE_FOLDER = "/Game/Abilities"
ASSET_NAME = "DT_AbilityCatalog"
PACKAGE_PATH = f"{PACKAGE_FOLDER}/{ASSET_NAME}"

# ---------------------------------------------------------------------------
# SYNC SOURCE — keep in lockstep with
#   src/components/modules/core-engine/sub_character/_shared/data.ts
# Tuple shape: (row_name, display_name, category, tier, base_damage, description, gameplay_tag)
# ---------------------------------------------------------------------------

CATALOG = [
    # ── Movement ──────────────────────────────────────────────────────────
    ("dodge",  "Dodge",  "Movement", 1, 0.0,
     "Roll/dash with invincibility frames.", "Ability.Movement.Dodge"),
    ("sprint", "Sprint", "Movement", 1, 0.0,
     "Hold to sprint at increased speed.", "Ability.Movement.Sprint"),

    # ── Melee ─────────────────────────────────────────────────────────────
    ("light_attack", "Light Attack", "Melee", 1, 20.0,
     "Quick saber strike.", "Ability.Melee.LightAttack"),
    ("heavy_attack", "Heavy Attack", "Melee", 2, 35.0,
     "Wind-up saber strike with stagger.", "Ability.Melee.HeavyAttack"),

    # ── Magical (Force Powers) ───────────────────────────────────────────
    ("force_push",      "Force Push",      "Magical", 1, 15.0,
     "Knockback shove.",                "Ability.Force.Push"),
    ("force_pull",      "Force Pull",      "Magical", 2, 10.0,
     "Pull target close.",              "Ability.Force.Pull"),
    ("force_heal",      "Force Heal",      "Magical", 3,  0.0,
     "Restore HP over time.",           "Ability.Force.Heal"),
    ("mind_trick",      "Mind Trick",      "Magical", 3,  0.0,
     "Persuade target NPC.",            "Ability.Force.MindTrick"),
    ("force_lightning", "Force Lightning", "Magical", 4, 60.0,
     "AoE chain damage.",               "Ability.Force.Lightning"),
    ("force_choke",     "Force Choke",     "Magical", 5, 80.0,
     "Hold + crushing damage.",         "Ability.Force.Choke"),

    # ── Utility ───────────────────────────────────────────────────────────
    ("quick_stim",    "Stim Pack",    "Utility", 1, 0.0,
     "Restore HP burst.",                "Item.QuickUse.StimPack"),
    ("quick_adrenal", "Adrenal",      "Utility", 2, 0.0,
     "Boost movement speed briefly.",    "Item.QuickUse.Adrenal"),
    ("force_focus",   "Force Focus",  "Utility", 2, 0.0,
     "Bullet-time aim assist.",          "Ability.Force.Focus"),
    ("interact",      "Interact",     "Utility", 1, 0.0,
     "Generic interaction (doors, NPCs, pickups).", "Ability.Utility.Interact"),
]

CATEGORY_ENUM_PATH = "/Script/PoF.EARPGAbilityCategory"
ROW_STRUCT_PATH    = "/Script/PoF.ARPGAbilityCatalogRow"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def ensure_folder(path):
    """Make sure the /Game/Abilities folder exists in the asset registry."""
    eal = unreal.EditorAssetLibrary
    if not eal.does_directory_exist(path):
        eal.make_directory(path)


def load_row_struct():
    s = unreal.load_object(None, ROW_STRUCT_PATH)
    if s is None:
        raise RuntimeError(
            f"Could not load USTRUCT {ROW_STRUCT_PATH}. "
            "Did you recompile after adding ARPGAbilityCatalogTypes.h?"
        )
    return s


def load_category_enum():
    e = unreal.load_object(None, CATEGORY_ENUM_PATH)
    if e is None:
        raise RuntimeError(
            f"Could not load UENUM {CATEGORY_ENUM_PATH}. Recompile required."
        )
    return e


def create_or_clear_datatable(row_struct):
    eal = unreal.EditorAssetLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()

    if eal.does_asset_exist(PACKAGE_PATH):
        dt = eal.load_asset(PACKAGE_PATH)
        # Clear all rows
        names = list(unreal.DataTableFunctionLibrary.get_data_table_row_names(dt))
        for n in names:
            unreal.DataTableFunctionLibrary.remove_data_table_row(dt, n)
        unreal.log(f"[seed_ability_catalog] Reused existing {PACKAGE_PATH} (cleared {len(names)} rows).")
        return dt

    factory = unreal.DataTableFactory()
    factory.struct = row_struct
    dt = tools.create_asset(ASSET_NAME, PACKAGE_FOLDER, unreal.DataTable, factory)
    if dt is None:
        raise RuntimeError(f"Failed to create DataTable at {PACKAGE_PATH}")
    unreal.log(f"[seed_ability_catalog] Created new {PACKAGE_PATH}.")
    return dt


def add_row(dt, row_name, display_name, category, tier, base_damage, description, gameplay_tag, cat_enum):
    """
    Adds one row. UE Python doesn't have a typed setter for arbitrary USTRUCTs;
    we use the JSON-import path via FillDataTableFromJSONString on a single-row
    array — works regardless of struct layout as long as field names match.
    """
    row_json = (
        "[{"
        f"\"Name\": \"{row_name}\","
        f"\"Name_Display\": \"{display_name}\","  # internal — not used; explicit
        # FName ⇒ string; EARPGAbilityCategory ⇒ enum display name
        f"\"Name\": \"{display_name}\","
        f"\"Category\": \"{category}\","
        f"\"Tier\": {tier},"
        f"\"BaseDamage\": {base_damage},"
        f"\"Description\": \"{description}\","
        f"\"GameplayTag\": {{\"TagName\": \"{gameplay_tag}\"}}"
        "}]"
    )
    # The simpler path: build the row programmatically via the Python proxy.
    row = unreal.ARPGAbilityCatalogRow()
    row.set_editor_property("Name", unreal.Name(display_name))
    enum_value = getattr(unreal.EARPGAbilityCategory, category.upper())
    row.set_editor_property("Category", enum_value)
    row.set_editor_property("Tier", int(tier))
    row.set_editor_property("BaseDamage", float(base_damage))
    row.set_editor_property("Description", unreal.Text(description))
    tag = unreal.GameplayTag()
    tag.tag_name = gameplay_tag
    row.set_editor_property("GameplayTag", tag)

    unreal.DataTableFunctionLibrary.add_data_table_row(dt, unreal.Name(row_name), row)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    unreal.log("[seed_ability_catalog] Starting.")
    ensure_folder(PACKAGE_FOLDER)
    row_struct = load_row_struct()
    cat_enum   = load_category_enum()

    dt = create_or_clear_datatable(row_struct)

    for (row_name, display_name, category, tier, base_damage, description, gameplay_tag) in CATALOG:
        try:
            add_row(dt, row_name, display_name, category, tier, base_damage, description, gameplay_tag, cat_enum)
            unreal.log(f"[seed_ability_catalog] + {row_name} ({category} t{tier}, {base_damage} dmg)")
        except Exception as exc:
            unreal.log_error(f"[seed_ability_catalog] FAILED row {row_name}: {exc}")

    unreal.EditorAssetLibrary.save_asset(PACKAGE_PATH)
    unreal.log(f"[seed_ability_catalog] Saved {PACKAGE_PATH} with {len(CATALOG)} rows.")


if __name__ == "__main__":
    main()

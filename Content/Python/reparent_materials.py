"""
reparent_materials.py
=====================
Consolidates the arena materials onto M_ARPG_Surface_Master without a visual
change. Creates MaterialInstanceConstants under /Game/Materials/, sets their
params from the existing /Game/ArenaBuild/Textures/T_* textures, and reassigns
SM_Arena's slots. Also creates MI_EnemyRed (red + emissive) as a consolidation
artifact, but does NOT swap it onto BP_VSEnemy -- the enemy's skeletal-mesh
material persistence is historically flaky (see fix_enemy_v*.py), so the working
/Game/VerticalSlice/M_EnemyRed is left in place to guarantee no regression.

Idempotent. Does NOT edit build_arena_ue.py / setup_characters_ue.py (parallel
branches own those).

Run headless: -ExecutePythonScript=<abs path>  (see build_master_material.py).
"""

import unreal

MASTER_PATH = "/Game/Materials/M_ARPG_Surface_Master"
SM_ARENA_PATH = "/Game/ArenaBuild/SM_Arena"
TEX = "/Game/ArenaBuild/Textures"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_lib = unreal.EditorAssetLibrary
mic_lib = unreal.MaterialEditingLibrary

# slot name on SM_Arena -> (instance path, texture surface prefix)
ARENA_SLOTS = {
    "M_Floor":  ("/Game/Materials/MI_Arena_Floor",  "floor"),
    "M_Wall":   ("/Game/Materials/MI_Arena_Wall",   "wall"),
    "M_Pillar": ("/Game/Materials/MI_Arena_Pillar", "pillar"),
}


def _log(msg):
    unreal.log("[reparent_materials] " + msg)


def split_path(p):
    idx = p.rfind("/")
    return p[:idx], p[idx + 1:]


def load(path):
    return asset_lib.load_asset(path) if asset_lib.does_asset_exist(path) else None


def make_instance(inst_path, master):
    folder, name = split_path(inst_path)
    if asset_lib.does_asset_exist(inst_path):
        asset_lib.delete_asset(inst_path)
    factory = unreal.MaterialInstanceConstantFactoryNew()
    inst = asset_tools.create_asset(name, folder, unreal.MaterialInstanceConstant, factory)
    mic_lib.set_material_instance_parent(inst, master)
    return inst


def set_tex(inst, param, tex_path):
    tex = load(tex_path)
    if tex is None:
        unreal.log_warning("[reparent_materials] missing texture " + tex_path)
        return
    mic_lib.set_material_instance_texture_parameter_value(inst, param, tex)


def main():
    _log("=== reparent START ===")
    master = load(MASTER_PATH)
    if master is None:
        raise RuntimeError("master missing; run build_master_material.py first")

    # --- Arena instances ------------------------------------------------------
    made = {}
    for slot, (inst_path, surface) in ARENA_SLOTS.items():
        inst = make_instance(inst_path, master)
        set_tex(inst, "Albedo",    "%s/T_%s_albedo" % (TEX, surface))
        set_tex(inst, "Normal",    "%s/T_%s_normal" % (TEX, surface))
        set_tex(inst, "Roughness", "%s/T_%s_rough" % (TEX, surface))
        mic_lib.set_material_instance_scalar_parameter_value(inst, "TilingScale", 1.0)
        mic_lib.set_material_instance_vector_parameter_value(
            inst, "BaseColorTint", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
        asset_lib.save_asset(inst_path)
        made[slot] = inst
        _log("Built " + inst_path)

    # --- Reassign SM_Arena slots ---------------------------------------------
    mesh = load(SM_ARENA_PATH)
    if mesh is None:
        unreal.log_warning("[reparent_materials] SM_Arena missing; skipping slot reassign")
    else:
        static_materials = mesh.get_editor_property("static_materials")
        for idx in range(len(static_materials)):
            slot = str(static_materials[idx].get_editor_property("material_slot_name"))
            inst = made.get(slot)
            if inst is not None:
                mesh.set_material(idx, inst)
                _log("Assigned %s -> slot[%d] '%s'" % (inst.get_path_name(), idx, slot))
            else:
                unreal.log_warning("[reparent_materials] no instance for slot '%s'" % slot)
        asset_lib.save_asset(SM_ARENA_PATH)

    # --- Enemy: MI_EnemyRed consolidation artifact (NOT swapped onto enemy) ---
    mi_enemy_path = "/Game/Materials/MI_EnemyRed"
    enemy_inst = make_instance(mi_enemy_path, master)
    mic_lib.set_material_instance_vector_parameter_value(
        enemy_inst, "BaseColorTint", unreal.LinearColor(0.8, 0.05, 0.05, 1.0))
    mic_lib.set_material_instance_scalar_parameter_value(enemy_inst, "EmissiveStrength", 1.5)
    asset_lib.save_asset(mi_enemy_path)
    _log("Built " + mi_enemy_path + " (artifact; enemy mesh left on existing M_EnemyRed)")

    _log("=== reparent COMPLETE ===")


if __name__ == "__main__":
    main()

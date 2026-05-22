"""
fix_enemy_v2.py
===============
Sets M_EnemyRed on BP_VSEnemy using a different persistence strategy:
1. Modifies the Blueprint CDO mesh override_materials.
2. Forces the package dirty via SaveLoadedAssets / OBJ SAVEPACKAGE console cmd.
3. Verifies by unloading and reloading the asset.
"""
import unreal

BP_VSENEMY_PATH = "/Game/VerticalSlice/BP_VSEnemy"
M_ENEMY_RED_PATH = "/Game/VerticalSlice/M_EnemyRed"
ENEMY_RED_RGB = (0.7, 0.04, 0.04)

asset_lib = unreal.EditorAssetLibrary
mat_lib = unreal.MaterialEditingLibrary
_LOG = []


def _log(msg):
    unreal.log_warning("[fix_v2] " + msg)
    _LOG.append(msg)


def split_path(p):
    i = p.rfind("/")
    return p[:i], p[i+1:]


def ensure_red_material():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    folder, name = split_path(M_ENEMY_RED_PATH)

    if asset_lib.does_asset_exist(M_ENEMY_RED_PATH):
        # Rename the old asset out of the way so we can recreate fresh.
        backup_path = M_ENEMY_RED_PATH + "_bak"
        if asset_lib.does_asset_exist(backup_path):
            asset_lib.delete_asset(backup_path)
        try:
            asset_lib.rename_asset(M_ENEMY_RED_PATH, backup_path)
            _log("Renamed old M_EnemyRed to _bak")
        except Exception as e:
            _log("rename failed (%s); trying direct delete" % str(e))
            asset_lib.delete_asset(M_ENEMY_RED_PATH)

    factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset(name, folder, unreal.Material, factory)
    if mat is None:
        raise RuntimeError("create_asset returned None for " + M_ENEMY_RED_PATH)
    _log("M_EnemyRed created fresh")

    r, g, b = ENEMY_RED_RGB
    # Base colour node — strong red.
    # NOTE: Constant3Vector output pin name is "" (empty string), NOT "RGB".
    color_node = mat_lib.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, -200)
    color_node.set_editor_property("constant", unreal.LinearColor(r, g, b, 1.0))
    ok1 = mat_lib.connect_material_property(color_node, "", unreal.MaterialProperty.MP_BASE_COLOR)
    _log("BaseColor connect result: %s" % str(ok1))

    # Roughness = 0.8 so the material diffuses light (avoids mirror-black look).
    rough_node = mat_lib.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 0)
    rough_node.set_editor_property("r", 0.8)
    ok2 = mat_lib.connect_material_property(rough_node, "", unreal.MaterialProperty.MP_ROUGHNESS)
    _log("Roughness connect result: %s" % str(ok2))

    # Emissive: strong self-glow so the enemy reads as clearly red regardless of lighting.
    # NOTE: Constant3Vector output pin name is "" (empty string), NOT "RGB".
    emissive_node = mat_lib.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 200)
    emissive_node.set_editor_property("constant", unreal.LinearColor(3.0, 0.0, 0.0, 1.0))
    ok3 = mat_lib.connect_material_property(emissive_node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    _log("Emissive connect result: %s" % str(ok3))

    mat.set_editor_property("used_with_skeletal_mesh", True)
    mat_lib.recompile_material(mat)
    asset_lib.save_asset(M_ENEMY_RED_PATH)
    _log("M_EnemyRed saved (roughness=0.8, emissive=2.0, used_with_skeletal_mesh=True)")
    return mat


def get_cdo_mesh(bp):
    gen_class = bp.generated_class()
    if gen_class is None:
        return None
    cdo = unreal.get_default_object(gen_class)
    if cdo is None:
        return None
    return getattr(cdo, "mesh", None)


def set_material_on_all(bp, mat):
    """Set override_materials on CDO mesh and SubobjectData template, both."""
    override_list = [mat, mat]  # 2 slots for SKM_Manny_Simple

    # CDO mesh.
    cdo_mesh = get_cdo_mesh(bp)
    if cdo_mesh is not None:
        cdo_mesh.set_editor_property("override_materials", override_list)
        _log("CDO mesh set. AFTER: %s" % [m.get_name() if m else "None" for m in
             (cdo_mesh.get_editor_property("override_materials") or [])])

    # SubobjectData template.
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    bfl = unreal.SubobjectDataBlueprintFunctionLibrary
    for h in subsystem.k2_gather_subobject_data_for_blueprint(bp):
        data = subsystem.k2_find_subobject_data_from_handle(h)
        if data is None:
            continue
        obj = bfl.get_associated_object(data)
        if obj is None or not isinstance(obj, unreal.SkeletalMeshComponent):
            continue
        obj.set_editor_property("override_materials", override_list)
        _log("Template component '%s' set. AFTER: %s" % (obj.get_name(),
             [m.get_name() if m else "None" for m in
              (obj.get_editor_property("override_materials") or [])]))


def force_save_blueprint(bp, mat):
    """
    Correct order:
      1. Compile the Blueprint (resets CDO override_materials to empty).
      2. Set override_materials on CDO + template AFTER compile.
      3. Save with save_packages_with_dialog (force, no compile).
      Never compile again after setting materials.
    """
    _log("Step 1: compile blueprint (resets CDO, then we override below)...")
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)

    _log("Step 2: re-set materials after compile...")
    set_material_on_all(bp, mat)

    _log("Step 3: force-save all dirty packages (no further compile)...")
    try:
        result = unreal.EditorLoadingAndSavingUtils.save_packages_with_dialog(
            packages_to_save=[], only_dirty=False)
        _log("save_packages_with_dialog result: %s" % str(result))
    except Exception as e:
        _log("save_packages_with_dialog failed: %s" % str(e))

    # Also try save_asset directly (belt-and-suspenders).
    saved = asset_lib.save_asset(BP_VSENEMY_PATH)
    _log("save_asset result: %s" % str(saved))


def main():
    _log("=== fix_enemy_v2 START ===")
    mat = ensure_red_material()

    bp = asset_lib.load_asset(BP_VSENEMY_PATH)
    if bp is None:
        raise RuntimeError("Blueprint not found")

    _log("Calling force_save_blueprint (compile → set materials → save)...")
    force_save_blueprint(bp, mat)

    # Final verify: unload and reload.
    _log("Verifying via unload + reload...")
    asset_lib.unload_asset(BP_VSENEMY_PATH)
    bp2 = asset_lib.load_asset(BP_VSENEMY_PATH)
    if bp2 is not None:
        cdo_mesh2 = get_cdo_mesh(bp2)
        if cdo_mesh2 is not None:
            try:
                mats = cdo_mesh2.get_editor_property("override_materials")
                _log("VERIFY: %s" % [m.get_name() if m else "None" for m in (mats or [])])
            except Exception as e:
                _log("VERIFY error: %s" % str(e))

    _log("=== fix_enemy_v2 COMPLETE ===")

    try:
        out = unreal.Paths.combine([unreal.Paths.project_saved_dir(), "fix_enemy_v2.log"])
        with open(out, "w", encoding="utf-8") as fh:
            fh.write("\n".join(_LOG) + "\n")
    except Exception:
        pass


if __name__ == "__main__":
    main()

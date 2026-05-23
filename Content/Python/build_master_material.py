"""
build_master_material.py
========================
Creates /Game/Materials/M_ARPG_Surface_Master -- a parameterised PBR master
material that M_Arena_* and M_EnemyRed reparent to (see reparent_materials.py).

Parameters:
  - Albedo / Normal / Roughness  (TextureSampleParameter2D)
  - TilingScale (ScalarParameter, default 1.0) * TextureCoordinate -> all UVs
  - BaseColorTint (VectorParameter, default white) * albedo -> Base Color
  - EmissiveStrength (ScalarParameter, default 0.0) * BaseColorTint -> Emissive

Branch #6 (textures & materials). Does NOT edit build_arena*.py /
setup_characters_ue.py (other branches own those).

Run headless (NOTE: -ExecutePythonScript, not -run=pythonscript):
    "C:\\Program Files\\Epic Games\\UE_5.7\\Engine\\Binaries\\Win64\\UnrealEditor.exe" ^
        "C:\\Users\\kazda\\Documents\\Unreal Projects\\PoF\\PoF.uproject" ^
        -ExecutePythonScript="<abs path to this file>" -unattended -nopause -nosplash
Headless editor exits with code 3 on a clean run (benign).
"""

import unreal

MASTER_PATH = "/Game/Materials/M_ARPG_Surface_Master"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_lib = unreal.EditorAssetLibrary
mat_lib = unreal.MaterialEditingLibrary


def _log(msg):
    unreal.log("[build_master_material] " + msg)


def split_path(package_path):
    idx = package_path.rfind("/")
    return package_path[:idx], package_path[idx + 1:]


def connect_uv(uv_expr, sampler):
    """Connect a UV-driver output to a sampler's coordinate input. The input pin
    name has varied across UE versions ('UVs' vs 'Coordinates'); try both."""
    for pin in ("UVs", "Coordinates"):
        try:
            if mat_lib.connect_material_expressions(uv_expr, "", sampler, pin):
                return True
        except Exception:
            pass
    unreal.log_warning("[build_master_material] could not connect UVs (pin name)")
    return False


def make_param_sampler(material, param_name, x, y, sampler_type, group="Textures"):
    node = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    node.set_editor_property("parameter_name", param_name)
    node.set_editor_property("group", group)
    node.set_editor_property("sampler_type", sampler_type)
    return node


def main():
    _log("=== build M_ARPG_Surface_Master START ===")
    folder, name = split_path(MASTER_PATH)

    if asset_lib.does_asset_exist(MASTER_PATH):
        asset_lib.delete_asset(MASTER_PATH)  # idempotent rebuild

    factory = unreal.MaterialFactoryNew()
    material = asset_tools.create_asset(name, folder, unreal.Material, factory)
    if material is None:
        raise RuntimeError("failed to create " + MASTER_PATH)

    # --- Tiling driver: TextureCoordinate * TilingScale -> sampler UVs --------
    texcoord = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionTextureCoordinate, -900, -100)
    tiling = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -900, 60)
    tiling.set_editor_property("parameter_name", "TilingScale")
    tiling.set_editor_property("default_value", 1.0)
    tiling.set_editor_property("group", "Tiling")
    uv_mul = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -700, -20)
    mat_lib.connect_material_expressions(texcoord, "", uv_mul, "A")
    mat_lib.connect_material_expressions(tiling, "", uv_mul, "B")

    # --- Albedo ---------------------------------------------------------------
    albedo = make_param_sampler(material, "Albedo", -400, -260,
                                unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    connect_uv(uv_mul, albedo)

    tint = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -400, -440)
    tint.set_editor_property("parameter_name", "BaseColorTint")
    tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    tint.set_editor_property("group", "Color")

    base_mul = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -150, -300)
    # VectorParameter output IS "RGB" (unlike Constant3Vector, whose pin is "").
    mat_lib.connect_material_expressions(albedo, "RGB", base_mul, "A")
    mat_lib.connect_material_expressions(tint, "RGB", base_mul, "B")
    mat_lib.connect_material_property(base_mul, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # --- Normal ---------------------------------------------------------------
    normal = make_param_sampler(material, "Normal", -400, 0,
                                unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    connect_uv(uv_mul, normal)
    mat_lib.connect_material_property(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)

    # --- Roughness ------------------------------------------------------------
    rough = make_param_sampler(material, "Roughness", -400, 260,
                               unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
    connect_uv(uv_mul, rough)
    mat_lib.connect_material_property(rough, "R", unreal.MaterialProperty.MP_ROUGHNESS)

    # --- Emissive (enemy): BaseColorTint * EmissiveStrength -> Emissive -------
    emissive_strength = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -400, 460)
    emissive_strength.set_editor_property("parameter_name", "EmissiveStrength")
    emissive_strength.set_editor_property("default_value", 0.0)
    emissive_strength.set_editor_property("group", "Emissive")
    emissive_mul = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -150, 400)
    mat_lib.connect_material_expressions(tint, "RGB", emissive_mul, "A")
    mat_lib.connect_material_expressions(emissive_strength, "", emissive_mul, "B")
    mat_lib.connect_material_property(emissive_mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    # Allow instances of this master to be used on skeletal meshes (the enemy).
    try:
        material.set_editor_property("used_with_skeletal_mesh", True)
    except Exception as exc:
        unreal.log_warning("[build_master_material] could not set skeletal usage: " + str(exc))

    mat_lib.recompile_material(material)
    asset_lib.save_asset(MASTER_PATH)

    # Diagnostic: list the exposed scalar + vector + texture params.
    scalars = mat_lib.get_scalar_parameter_names(material)
    vectors = mat_lib.get_vector_parameter_names(material)
    textures = mat_lib.get_texture_parameter_names(material)
    _log("Scalar params: " + ", ".join(str(s) for s in scalars))
    _log("Vector params: " + ", ".join(str(v) for v in vectors))
    _log("Texture params: " + ", ".join(str(t) for t in textures))
    _log("=== build M_ARPG_Surface_Master COMPLETE: " + MASTER_PATH + " ===")


if __name__ == "__main__":
    main()

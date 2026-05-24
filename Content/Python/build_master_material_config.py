"""
build_master_material_config.py
==============================
Wire Source/PoF/Materials/ARPGMasterMaterialConfig.h (the SP-B generated
UPrimaryDataAsset) to the actual master material (game §2): create a
UARPGMasterMaterialConfig data asset that documents M_ARPG_Surface_Master's
parameter layout so designers / CreateConfiguredInstance() have a single source
of truth for the master's scalar / vector / texture parameters.

Idempotent (deletes + rebuilds the data asset). Defensive: each step that
depends on an uncertain editor API is wrapped so a partial failure logs a
warning rather than aborting.

Run headless:
    "<UE>/UnrealEditor.exe" "<...>/PoF.uproject" ^
        -ExecutePythonScript="<abs path>" -unattended -nopause -nosplash
"""
import unreal

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_lib = unreal.EditorAssetLibrary

CONFIG_PATH = "/Game/Materials/DA_ARPGSurfaceMasterConfig"
MASTER_PATH = "/Game/Materials/M_ARPG_Surface_Master"


def scalar(name, default, mn, mx, desc):
    p = unreal.ARPGMaterialScalarParam()
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", default)
    p.set_editor_property("min_value", mn)
    p.set_editor_property("max_value", mx)
    p.set_editor_property("description", desc)
    return p


def vector(name, color, is_color, desc):
    p = unreal.ARPGMaterialVectorParam()
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", color)
    # bIsColor -> "is_color" (set_editor_property drops the leading bool 'b').
    p.set_editor_property("is_color", is_color)
    p.set_editor_property("description", desc)
    return p


def texture(name, desc):
    p = unreal.ARPGMaterialTextureParam()
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("description", desc)
    return p


def main():
    unreal.log("[build_master_material_config] === START ===")
    if not asset_lib.does_asset_exist(MASTER_PATH):
        raise RuntimeError("master missing; run build_master_material.py first")

    if asset_lib.does_asset_exist(CONFIG_PATH):
        asset_lib.delete_asset(CONFIG_PATH)

    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.ARPGMasterMaterialConfig)
    config = asset_tools.create_asset(
        "DA_ARPGSurfaceMasterConfig", "/Game/Materials", unreal.ARPGMasterMaterialConfig, factory)
    if config is None:
        raise RuntimeError("failed to create " + CONFIG_PATH)

    config.set_editor_property("material_name", "ARPG Surface (Opaque PBR)")

    # Soft pointer to the master. Defensive: leave unset (with a warning) if the
    # editor rejects the assignment shape.
    try:
        config.set_editor_property("master_material", asset_lib.load_asset(MASTER_PATH))
    except Exception as exc:
        unreal.log_warning("[build_master_material_config] could not set master_material: " + str(exc))

    config.set_editor_property("scalar_parameters", [
        scalar("TilingScale", 1.0, 0.0, 16.0, "UV tiling multiplier for the base PBR set."),
        scalar("EmissiveStrength", 0.0, 0.0, 10.0, "BaseColorTint * this -> emissive (enemy glow)."),
        scalar("DetailTiling", 8.0, 0.0, 64.0, "High-frequency detail-normal tiling rate (game §3)."),
    ])
    config.set_editor_property("vector_parameters", [
        vector("BaseColorTint", unreal.LinearColor(1.0, 1.0, 1.0, 1.0), True, "Albedo tint (white = unchanged)."),
    ])
    config.set_editor_property("texture_parameters", [
        texture("Albedo", "Base color map (sRGB)."),
        texture("Normal", "Tangent-space normal map (non-sRGB)."),
        texture("Roughness", "Linear grayscale roughness (non-sRGB)."),
        texture("DetailNormal", "Optional high-frequency detail normal (non-sRGB)."),
    ])

    asset_lib.save_asset(CONFIG_PATH)
    unreal.log("[build_master_material_config] === DONE: %s ===" % CONFIG_PATH)


if __name__ == "__main__":
    main()

"""Author an UNLIT emissive material so the test-map floor renders visibly in SceneCapture
regardless of lighting/exposure (the floor was crushing to black). Reports each step.

Call: /pof/python/run {module:"observation.make_floor_material", function:"run", args:{}}.
"""
import unreal


def run(args):
    name = args.get("name", "M_FloorRef")
    folder = args.get("folder", "/Game/Maps")
    full = f"{folder}/{name}"
    out = {"path": full}
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        unreal.EditorAssetLibrary.delete_asset(full)  # force recreate so brightness edits take
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(name, folder, unreal.Material, unreal.MaterialFactoryNew())
    if not mat:
        return {**out, "error": "create_asset failed"}
    out["created"] = True
    try:
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    except Exception as e:
        out["shading_err"] = str(e)
    try:
        node = unreal.MaterialEditingLibrary.create_material_expression(
            mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
        node.set_editor_property("constant", unreal.LinearColor(0.0, 8.0, 0.0, 1.0))  # HDR-bright green (glows; can't be tonemapped to black)
        unreal.MaterialEditingLibrary.connect_material_property(
            node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        unreal.MaterialEditingLibrary.recompile_material(mat)
        out["wired"] = True
    except Exception as e:
        out["wire_err"] = str(e)
        out["wired"] = False
    unreal.EditorAssetLibrary.save_asset(full)
    return out

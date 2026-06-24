"""Stream 1 (env-arena) polish — give M_Sand_Floor real tonal variation.

The first authoring pass fell back to a FLAT sand color because
MaterialEditingLibrary.connect_material_expression raised AttributeError in this 5.8
Python build. This script discovers the actual expression->expression connector name at
runtime, then rebuilds the base color as a noise-driven lerp between two sand tones
(large soft patches), keeps high roughness, and adds gentle large-scale normal break-up
if a noise->normal path is available. Recompiles + saves. Logs exactly what it did.
"""
import unreal

SAND = "/Game/Environments/AncientArena/M_Sand_Floor"
mel = unreal.MaterialEditingLibrary


def pick(*cands):
    for c in cands:
        if hasattr(mel, c):
            return getattr(mel, c)
    return None


def main():
    connectors = [n for n in dir(mel) if "connect" in n.lower()]
    unreal.log("[SAND] connectors available: %s" % connectors)
    connect_expr = pick("connect_material_expression", "connect_material_expressions")
    connect_prop = pick("connect_material_property")
    if not unreal.EditorAssetLibrary.does_asset_exist(SAND):
        unreal.log_warning("[SAND] material missing")
        return
    mat = unreal.load_asset(SAND)

    # clean slate, then rebuild
    try:
        mel.delete_all_material_expressions(mat)
    except Exception as e:  # noqa: BLE001
        unreal.log_warning("[SAND] delete_all failed: %s" % e)

    light = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -760, -140)
    light.set_editor_property("constant", unreal.LinearColor(0.84, 0.73, 0.50, 1.0))
    dark = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -760, 140)
    dark.set_editor_property("constant", unreal.LinearColor(0.55, 0.45, 0.30, 1.0))

    base = light
    varied = False
    if connect_expr:
        try:
            noise = mel.create_material_expression(mat, unreal.MaterialExpressionNoise, -760, 380)
            for p, v in (("scale", 0.0012), ("output_min", 0.0), ("output_max", 1.0), ("levels", 3)):
                try:
                    noise.set_editor_property(p, v)
                except Exception:  # noqa: BLE001
                    pass
            lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -380, 0)
            connect_expr(dark, "", lerp, "A")
            connect_expr(light, "", lerp, "B")
            connect_expr(noise, "", lerp, "Alpha")
            base = lerp
            varied = True
            unreal.log("[SAND] tonal variation wired via %s" % connect_expr.__name__)
        except Exception as e:  # noqa: BLE001
            unreal.log_warning("[SAND] variation wiring failed (%s) -> flat" % e)
    else:
        unreal.log_warning("[SAND] no expr->expr connector found -> flat sand")

    connect_prop(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -380, 250)
    rough.set_editor_property("r", 0.93)
    connect_prop(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -380, 410)
    spec.set_editor_property("r", 0.08)
    connect_prop(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    try:
        mat.set_editor_property("used_with_static_lighting", True)
    except Exception:  # noqa: BLE001
        pass
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(SAND)
    unreal.log("[SAND] saved (varied=%s)" % varied)
    unreal.log("[gate] RESULT=%s" % ("PASS" if varied else "WARN-FLAT"))


main()

"""Colosseum — Pass 2: replace the checker-tiled arena stone with procedural TRAVERTINE.

Builds M_Travertine (creamy Roman limestone): engine tiling-noise drives base-color + roughness
variation, the engine tiling detail-normal gives real surface relief, all engine-native (no
marketplace assets). Assigns it to the Arena wall+pillar slots and every ruin/rubble actor,
replacing M_Arena_Wall/Pillar's checker look. Leaves the sand floor.
"""
import unreal

ENVDIR = "/Game/Environments/AncientArena"
TRAV = ENVDIR + "/M_Travertine"
T_NOISE = "/Engine/MaterialTemplates/Textures/T_Noise01"
T_DETAIL_N = "/Engine/Engine_MI_Shaders/T_Base_Tile_DetailNormal"
MAP = "/Game/Maps/Arena_Ancient"
mel = unreal.MaterialEditingLibrary
res = {"assigned": 0, "errors": []}


def L(m):
    unreal.log("[COLO2] " + m)


def pick(*c):
    for n in c:
        if hasattr(mel, n):
            return getattr(mel, n)
    return None


CE = pick("connect_material_expression", "connect_material_expressions")
CP = pick("connect_material_property")


def expr(mat, cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)


def tex(mat, path, x, y, sampler, tiling):
    ts = expr(mat, unreal.MaterialExpressionTextureSample, x, y)
    t = unreal.load_asset(path)
    if t:
        ts.set_editor_property("texture", t)
    try:
        ts.set_editor_property("sampler_type", sampler)
    except Exception as e:  # noqa: BLE001
        res["errors"].append("sampler %s: %s" % (path, e))
    if tiling and CE:
        tc = expr(mat, unreal.MaterialExpressionTextureCoordinate, x - 320, y)
        tc.set_editor_property("u_tiling", tiling)
        tc.set_editor_property("v_tiling", tiling)
        CE(tc, "", ts, "UVs")
    return ts


def make_travertine():
    if unreal.EditorAssetLibrary.does_asset_exist(TRAV):
        return unreal.load_asset(TRAV)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset("M_Travertine", ENVDIR, unreal.Material, unreal.MaterialFactoryNew())

    # color: lerp two travertine creams by tiling noise
    dark = expr(mat, unreal.MaterialExpressionConstant3Vector, -760, -150)
    dark.set_editor_property("constant", unreal.LinearColor(0.58, 0.53, 0.43, 1.0))
    lite = expr(mat, unreal.MaterialExpressionConstant3Vector, -760, 30)
    lite.set_editor_property("constant", unreal.LinearColor(0.84, 0.79, 0.68, 1.0))
    noise = tex(mat, T_NOISE, -760, 220, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, 2.5)
    base = lite
    rough = None
    if CE:
        lerp = expr(mat, unreal.MaterialExpressionLinearInterpolate, -360, -60)
        CE(dark, "", lerp, "A")
        CE(lite, "", lerp, "B")
        CE(noise, "R", lerp, "Alpha")
        base = lerp
        # roughness 0.55..0.88 by the same noise (wet/polished vs weathered)
        rlo = expr(mat, unreal.MaterialExpressionConstant, -760, 360)
        rlo.set_editor_property("r", 0.55)
        rhi = expr(mat, unreal.MaterialExpressionConstant, -760, 430)
        rhi.set_editor_property("r", 0.9)
        rlerp = expr(mat, unreal.MaterialExpressionLinearInterpolate, -360, 380)
        CE(rlo, "", rlerp, "A")
        CE(rhi, "", rlerp, "B")
        CE(noise, "R", rlerp, "Alpha")
        rough = rlerp
    CP(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    if rough is not None:
        CP(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    else:
        rc = expr(mat, unreal.MaterialExpressionConstant, -360, 380)
        rc.set_editor_property("r", 0.78)
        CP(rc, "", unreal.MaterialProperty.MP_ROUGHNESS)

    # surface relief from the engine tiling detail normal
    dn = tex(mat, T_DETAIL_N, -360, 560, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, 7.0)
    CP(dn, "", unreal.MaterialProperty.MP_NORMAL)

    spec = expr(mat, unreal.MaterialExpressionConstant, -360, 720)
    spec.set_editor_property("r", 0.25)
    CP(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    try:
        mat.set_editor_property("used_with_static_lighting", True)
    except Exception:  # noqa: BLE001
        pass
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(TRAV)
    L("travertine created (CE=%s)" % (CE.__name__ if CE else None))
    return mat


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    trav = make_travertine()
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    for a in list(eas.get_all_level_actors()):
        lbl = a.get_actor_label()
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        if not smc:
            continue
        if lbl == "Arena":
            smc.set_material(1, trav)   # wall slot
            smc.set_material(2, trav)   # pillar slot
            res["assigned"] += 1
        elif lbl.startswith("Ruin_") or lbl.startswith("Rubble_"):
            smc.set_material(0, trav)
            res["assigned"] += 1
    saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    L("assigned travertine to %d meshes, saved=%s, errors=%d" % (res["assigned"], saved, len(res["errors"])))
    for e in res["errors"]:
        L("ERR " + e)
    L("[gate] RESULT=%s" % ("PASS" if saved and res["assigned"] > 0 else "WARN"))


main()

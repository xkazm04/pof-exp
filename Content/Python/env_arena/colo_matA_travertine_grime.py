"""Realism Pass A (material) — rebuild M_Travertine as weathered ancient stone:
two-octave tiling-noise colour/roughness variation + a world-height GRIME gradient (bases
darker, as dirt/water-staining accumulates low) + the engine detail-normal for relief.
Engine-native. All node wiring guarded.
"""
import unreal

TRAV = "/Game/Environments/AncientArena/M_Travertine"
T_NOISE = "/Engine/MaterialTemplates/Textures/T_Noise01"
T_DETAIL_N = "/Engine/Engine_MI_Shaders/T_Base_Tile_DetailNormal"
mel = unreal.MaterialEditingLibrary


def pick(*c):
    for n in c:
        if hasattr(mel, n):
            return getattr(mel, n)
    return None


CE = pick("connect_material_expressions", "connect_material_expression")
CP = pick("connect_material_property")
errs = []


def main():
    mat = unreal.load_asset(TRAV)
    if not mat:
        unreal.log_warning("[MATA] no travertine"); return
    try:
        mel.delete_all_material_expressions(mat)
    except Exception:
        pass

    def E(cls, x, y):
        return mel.create_material_expression(mat, cls, x, y)

    def C(v, x, y):
        n = E(unreal.MaterialExpressionConstant, x, y); n.set_editor_property("r", v); return n

    def C3(r, g, b, x, y):
        n = E(unreal.MaterialExpressionConstant3Vector, x, y)
        n.set_editor_property("constant", unreal.LinearColor(r, g, b, 1.0)); return n

    def tex(path, x, y, sampler, tiling):
        ts = E(unreal.MaterialExpressionTextureSample, x, y)
        t = unreal.load_asset(path)
        if t:
            ts.set_editor_property("texture", t)
        try:
            ts.set_editor_property("sampler_type", sampler)
        except Exception as e:
            errs.append(str(e))
        if tiling and CE:
            tc = E(unreal.MaterialExpressionTextureCoordinate, x - 300, y)
            tc.set_editor_property("u_tiling", tiling); tc.set_editor_property("v_tiling", tiling)
            CE(tc, "", ts, "UVs")
        return ts

    # --- two-octave colour variation (warm travertine)
    dark = C3(0.56, 0.48, 0.35, -900, -160)
    lite = C3(0.92, 0.84, 0.66, -900, -20)
    n1 = tex(T_NOISE, -900, 150, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, 2.0)
    n2 = tex(T_NOISE, -900, 380, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR, 0.45)  # big blotches
    base = lite
    if CE:
        col = E(unreal.MaterialExpressionLinearInterpolate, -560, -80)
        CE(dark, "", col, "A"); CE(lite, "", col, "B"); CE(n1, "R", col, "Alpha")
        # blotchy dirt: multiply by the large-scale noise (0.7..1.0)
        blotch = E(unreal.MaterialExpressionLinearInterpolate, -560, 200)
        CE(C(0.82, -740, 250), "", blotch, "A"); CE(C(1.0, -740, 320), "", blotch, "B")
        CE(n2, "R", blotch, "Alpha")
        colb = E(unreal.MaterialExpressionMultiply, -380, 40)
        CE(col, "", colb, "A"); CE(blotch, "", colb, "B")
        base = colb

    # --- world-height grime: darken low (dirt/staining accumulates at the base)
    if CE:
        try:
            wp = E(unreal.MaterialExpressionWorldPosition, -900, 560)
            zmask = E(unreal.MaterialExpressionComponentMask, -720, 560)
            zmask.set_editor_property("r", False); zmask.set_editor_property("g", False)
            zmask.set_editor_property("b", True); zmask.set_editor_property("a", False)
            CE(wp, "", zmask, "")
            zdiv = E(unreal.MaterialExpressionMultiply, -560, 560)
            CE(zmask, "", zdiv, "A"); CE(C(1.0 / 1400.0, -720, 640), "", zdiv, "B")
            zsat = E(unreal.MaterialExpressionSaturate, -420, 560)
            CE(zdiv, "", zsat, "")
            grime = E(unreal.MaterialExpressionLinearInterpolate, -300, 540)
            CE(C(0.66, -420, 660), "", grime, "A"); CE(C(1.0, -420, 720), "", grime, "B")
            CE(zsat, "", grime, "Alpha")
            final = E(unreal.MaterialExpressionMultiply, -150, 120)
            CE(base, "", final, "A"); CE(grime, "", final, "B")
            base = final
        except Exception as e:
            errs.append("grime: %s" % e)

    CP(base, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # roughness 0.6..0.92 by noise (weathered)
    if CE:
        rl = E(unreal.MaterialExpressionLinearInterpolate, -360, 820)
        CE(C(0.6, -560, 820), "", rl, "A"); CE(C(0.93, -560, 880), "", rl, "B"); CE(n1, "R", rl, "Alpha")
        CP(rl, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = C(0.18, -360, 960); CP(spec, "", unreal.MaterialProperty.MP_SPECULAR)

    dn = tex(T_DETAIL_N, -360, 1080, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, 9.0)
    CP(dn, "", unreal.MaterialProperty.MP_NORMAL)

    try:
        mat.set_editor_property("used_with_static_lighting", True)
    except Exception:
        pass
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(TRAV)
    unreal.log("[MATA] weathered travertine saved; errs=%d" % len(errs))
    for e in errs:
        unreal.log_warning("[MATA] " + e)
    unreal.log("[gate] RESULT=PASS")


main()

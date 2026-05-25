"""
build_weathered_stone.py
========================
Catalog-pipeline row: Core/Existing -> Material -> target asset "Weathered Stone".

Builds /Game/Materials/MI_WeatheredStone -- a MaterialInstanceConstant parented
to the shared surface master M_ARPG_Surface_Master (build_master_material.py).
It REUSES the master rather than authoring a new material, and exercises the two
master features the arena instances leave unset, so it reads as aged stone:

  * DetailNormal + DetailTiling  -> angle-corrected micro-normal breakup
    (the master's detail-normal path; MI_Arena_* never assign it).
  * BaseColorTint                -> a desaturated, slightly-darkened grey-tan
    that ages the albedo without a new texture.

The parameter set is the SYNC MIRROR of the app-side catalog spec
(src/lib/catalog/seed-materials.ts, entity `mat-weathered-stone`). The app is the
sync source -- if the spec there changes, re-run this script (seed_ability_catalog
convention).

The script is its own TEST GATE: after building it self-validates the resulting
instance (parent, texture/scalar/vector overrides, non-sRGB normal+roughness) and
raises SystemExit on any failure. Judge success by the line `[gate] RESULT=PASS`
in the -abslog, NOT the process exit code (the headless editor exits non-zero on a
benign shutdown crash).

Idempotent: deletes + recreates the instance each run. Touches only its own asset
(no shared .umap / master edits), so it is safe on the shared UE tree.

Run headless (NOTE: -ExecutePythonScript, not -run=pythonscript):
    "C:\\Program Files\\Epic Games\\UE_5.7\\Engine\\Binaries\\Win64\\UnrealEditor.exe" ^
        "C:\\Users\\kazda\\Documents\\Unreal Projects\\PoF\\PoF.uproject" ^
        -ExecutePythonScript="<abs path to this file>" ^
        -unattended -nopause -nosplash ^
        -abslog="<abs path>\\Saved\\Logs\\build_weathered_stone.log"
"""

import unreal

MASTER_PATH = "/Game/Materials/M_ARPG_Surface_Master"
INSTANCE_PATH = "/Game/Materials/MI_WeatheredStone"
TEX = "/Game/ArenaBuild/Textures"  # stone arena surfaces (build_arena/reparent)

# --- Design spec (mirror of the app catalog entity `mat-weathered-stone`) -----
TEXTURES = {
    "Albedo":       "%s/T_wall_albedo" % TEX,
    "Normal":       "%s/T_wall_normal" % TEX,
    "Roughness":    "%s/T_wall_rough"  % TEX,
    "DetailNormal": "%s/T_wall_normal" % TEX,  # reuse normal as high-tiling breakup
}
NON_SRGB_PARAMS = ("Normal", "Roughness", "DetailNormal")  # PS-2 render gotcha
SCALARS = {
    "TilingScale":      1.0,
    "DetailTiling":     8.0,   # micro weathering breakup
    "EmissiveStrength": 0.0,   # stone does not glow
}
BASE_COLOR_TINT = unreal.LinearColor(0.72, 0.70, 0.64, 1.0)  # aged grey-tan
TINT_TOLERANCE = 0.01

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_lib = unreal.EditorAssetLibrary
mic_lib = unreal.MaterialEditingLibrary


def _log(msg):
    unreal.log("[build_weathered_stone] " + msg)


def _fail(msg):
    unreal.log_error("[build_weathered_stone] " + msg)
    unreal.log("[gate] RESULT=FAIL " + msg)
    raise SystemExit("[build_weathered_stone] " + msg)


def split_path(p):
    idx = p.rfind("/")
    return p[:idx], p[idx + 1:]


def load(path):
    return asset_lib.load_asset(path) if asset_lib.does_asset_exist(path) else None


def build():
    _log("=== build MI_WeatheredStone START ===")
    master = load(MASTER_PATH)
    if master is None:
        _fail("master missing at %s; run build_master_material.py first" % MASTER_PATH)

    if asset_lib.does_asset_exist(INSTANCE_PATH):
        asset_lib.delete_asset(INSTANCE_PATH)  # idempotent rebuild

    folder, name = split_path(INSTANCE_PATH)
    factory = unreal.MaterialInstanceConstantFactoryNew()
    inst = asset_tools.create_asset(name, folder, unreal.MaterialInstanceConstant, factory)
    if inst is None:
        _fail("failed to create %s" % INSTANCE_PATH)
    mic_lib.set_material_instance_parent(inst, master)

    for param, tex_path in TEXTURES.items():
        tex = load(tex_path)
        if tex is None:
            _fail("missing texture %s for param %s" % (tex_path, param))
        mic_lib.set_material_instance_texture_parameter_value(inst, param, tex)

    for param, value in SCALARS.items():
        mic_lib.set_material_instance_scalar_parameter_value(inst, param, value)

    mic_lib.set_material_instance_vector_parameter_value(inst, "BaseColorTint", BASE_COLOR_TINT)

    asset_lib.save_asset(INSTANCE_PATH)
    _log("Built " + INSTANCE_PATH)
    return inst


# --- Gate: validate the built instance against the design spec ----------------

def _overrides_by_name(values):
    """{param name -> parameter_value} for a UMaterialInstance *_parameter_values array."""
    out = {}
    for v in values:
        info = v.get_editor_property("parameter_info")
        out[str(info.get_editor_property("name"))] = v.get_editor_property("parameter_value")
    return out


def validate():
    _log("--- gate: validate MI_WeatheredStone ---")
    inst = load(INSTANCE_PATH)
    if inst is None:
        _fail("instance missing at %s after build" % INSTANCE_PATH)

    # #1 parent is the shared master (reuse invariant)
    parent = inst.get_editor_property("parent")
    if parent is None or not parent.get_path_name().startswith(MASTER_PATH):
        _fail("parent is %s, expected %s" % (
            parent.get_path_name() if parent else "None", MASTER_PATH))
    _log("#1 parent OK -> " + parent.get_path_name())

    # #2 every spec texture param resolves
    tex_over = _overrides_by_name(inst.get_editor_property("texture_parameter_values"))
    for param in TEXTURES:
        tex = tex_over.get(param)
        if tex is None:
            _fail("texture param %s not assigned" % param)
    _log("#2 texture params OK -> " + ", ".join(sorted(tex_over.keys())))

    # #3 normal/roughness/detail sampled non-sRGB (PS-2 wash-out guard)
    srgb_violations = []
    for param in NON_SRGB_PARAMS:
        tex = tex_over.get(param)
        if tex is not None and bool(tex.get_editor_property("srgb")):
            srgb_violations.append(param)
    if srgb_violations:
        _fail("sRGB violations on non-color params: " + ", ".join(srgb_violations))
    _log("#3 non-sRGB OK for " + ", ".join(NON_SRGB_PARAMS))

    # #4 scalar overrides match the spec
    sc_over = _overrides_by_name(inst.get_editor_property("scalar_parameter_values"))
    for param, expected in SCALARS.items():
        got = sc_over.get(param)
        if got is None or abs(float(got) - expected) > 1e-4:
            _fail("scalar %s = %s, expected %s" % (param, got, expected))
    _log("#4 scalar params OK -> " + ", ".join("%s=%s" % (k, SCALARS[k]) for k in SCALARS))

    # #5 BaseColorTint matches the designed weathered tint
    vec_over = _overrides_by_name(inst.get_editor_property("vector_parameter_values"))
    tint = vec_over.get("BaseColorTint")
    if tint is None:
        _fail("BaseColorTint not assigned")
    for chan, exp in (("r", BASE_COLOR_TINT.r), ("g", BASE_COLOR_TINT.g), ("b", BASE_COLOR_TINT.b)):
        if abs(tint.get_editor_property(chan) - exp) > TINT_TOLERANCE:
            _fail("BaseColorTint.%s = %s, expected %s" % (chan, tint.get_editor_property(chan), exp))
    _log("#5 BaseColorTint OK -> (%.2f, %.2f, %.2f)" % (
        tint.get_editor_property("r"), tint.get_editor_property("g"), tint.get_editor_property("b")))

    _log("[gate] RESULT=PASS MI_WeatheredStone verified (parent + 4 tex + 3 scalar + tint, non-sRGB)")


def main():
    build()
    validate()
    _log("=== build MI_WeatheredStone COMPLETE: " + INSTANCE_PATH + " ===")


if __name__ == "__main__":
    main()

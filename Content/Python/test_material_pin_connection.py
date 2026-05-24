"""
test_material_pin_connection.py
===============================
Codifies the Constant3Vector output-pin gotcha (folder-06 tests.md UE #2) as a
deterministic editor automation check.

`MaterialExpressionConstant3Vector`'s output pin is "" (empty string), NOT "RGB".
`connect_material_property(node, "RGB", ...)` therefore silently returns False and
produces a black material. This reproduces it: connecting with "" succeeds, and
connecting with "RGB" fails.

Implemented in Python (not a C++ AFunctionalTest) on purpose: it needs the
editor-only `MaterialEditingLibrary`, and the runtime PoF module deliberately does
NOT depend on UnrealEd/MaterialEditor — adding that dependency would be a risky
change to a shared build. Python runs only in the editor, so there is zero
compile-time risk.

Run headless (exit code 0 = pass; non-zero = a failed assertion):
    "<UE>/UnrealEditor.exe" "<...>/PoF.uproject" ^
        -ExecutePythonScript="<abs path to this file>" -unattended -nopause -nosplash
"""

import unreal

TMP_PATH = "/Game/Materials/__pin_connection_test_tmp"


def _log(msg):
    unreal.log("[test_material_pin] " + msg)


def main():
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset_lib = unreal.EditorAssetLibrary
    mat_lib = unreal.MaterialEditingLibrary

    if asset_lib.does_asset_exist(TMP_PATH):
        asset_lib.delete_asset(TMP_PATH)

    factory = unreal.MaterialFactoryNew()
    material = asset_tools.create_asset("__pin_connection_test_tmp", "/Game/Materials", unreal.Material, factory)
    if material is None:
        raise SystemExit("test_material_pin_connection FAILED: could not create scratch material")

    failures = []
    try:
        c3v = mat_lib.create_material_expression(
            material, unreal.MaterialExpressionConstant3Vector, -300, 0)
        c3v.set_editor_property("constant", unreal.LinearColor(0.7, 0.04, 0.04, 1.0))

        # CORRECT: Constant3Vector's single output pin is the empty string.
        ok_empty = mat_lib.connect_material_property(c3v, "", unreal.MaterialProperty.MP_BASE_COLOR)
        if not ok_empty:
            failures.append("connect_material_property(c3v, '', MP_BASE_COLOR) returned False (expected True)")

        # WRONG: "RGB" is not a valid output pin of Constant3Vector -> returns False.
        # Use a different property so this is not confused with 'already connected'.
        bad_rgb = mat_lib.connect_material_property(c3v, "RGB", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
        if bad_rgb:
            failures.append(
                "connect_material_property(c3v, 'RGB', MP_EMISSIVE_COLOR) returned True "
                "(expected False — Constant3Vector output pin is '' not 'RGB')")
    finally:
        asset_lib.delete_asset(TMP_PATH)

    if failures:
        for f in failures:
            unreal.log_error("[test_material_pin] FAIL: " + f)
        raise SystemExit("test_material_pin_connection FAILED: %d assertion(s)" % len(failures))

    _log("PASS: Constant3Vector output pin is '' (empty); 'RGB' is rejected as expected.")


if __name__ == "__main__":
    main()

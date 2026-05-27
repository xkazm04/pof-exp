"""build_anim_bp drives the PoFAnimBPAuthoringLibrary in the right order."""

import unreal

from player_movement import build_anim_bp


def setup_function(_fn):
    unreal.EditorAssetLibrary._present.clear()
    unreal.EditorAssetLibrary._assets.clear()
    unreal.PoFAnimBPAuthoringLibrary.calls.clear()

    skel = build_anim_bp.SKEL_CANDIDATES[0]
    unreal.EditorAssetLibrary._present.add(skel)
    unreal.EditorAssetLibrary._assets[skel] = object()
    unreal.EditorAssetLibrary._present.add(build_anim_bp.BS_PATH)
    unreal.EditorAssetLibrary._assets[build_anim_bp.BS_PATH] = object()


def test_calls_library_in_documented_order():
    result = build_anim_bp.run({})
    names = [c[0] for c in unreal.PoFAnimBPAuthoringLibrary.calls]
    assert names == ["create", "sm", "bss", "slot", "connect", "compile"]
    assert "ABP_VSPlayer" in result["created"]


def test_missing_library_fails_cleanly():
    saved = unreal.PoFAnimBPAuthoringLibrary
    del unreal.PoFAnimBPAuthoringLibrary
    try:
        result = build_anim_bp.run({})
        assert any("PoFAnimBPAuthoringLibrary not available" in f for f in result["failed"])
    finally:
        unreal.PoFAnimBPAuthoringLibrary = saved


def test_missing_skeleton_fails_cleanly():
    for path in build_anim_bp.SKEL_CANDIDATES:
        unreal.EditorAssetLibrary._present.discard(path)
        unreal.EditorAssetLibrary._assets.pop(path, None)
    result = build_anim_bp.run({})
    assert any("Manny skeleton" in f for f in result["failed"])

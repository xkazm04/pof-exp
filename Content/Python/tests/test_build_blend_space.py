"""Blend-space grid wiring."""

import types
import unreal

from player_movement import build_blend_space


def setup_function(_fn):
    unreal.EditorAssetLibrary._present.clear()
    unreal.EditorAssetLibrary._assets.clear()
    unreal.BlendSpaceLibrary._samples.clear()

    # BS exists
    bs_obj = object()
    unreal.EditorAssetLibrary._present.add(build_blend_space.BS_PATH)
    unreal.EditorAssetLibrary._assets[build_blend_space.BS_PATH] = bs_obj

    # All 9 unique retargeted clips exist
    unique_clips = {b for _, _, b in build_blend_space.GRID}
    for b in unique_clips:
        path = f"{build_blend_space.RT_DIR}/{b}_RT"
        unreal.EditorAssetLibrary._present.add(path)
        unreal.EditorAssetLibrary._assets[path] = object()


def test_grid_has_11_samples():
    result = build_blend_space.run({})
    assert result["sample_count"] == 11
    assert result["failed"] == []


def test_missing_clip_surfaces_in_failed_list():
    # Drop one retargeted clip
    unreal.EditorAssetLibrary._present.discard(f"{build_blend_space.RT_DIR}/Walking_RT")
    result = build_blend_space.run({})
    assert any("Walking_RT" in f for f in result["failed"])


def test_missing_bs_returns_failure():
    unreal.EditorAssetLibrary._present.discard(build_blend_space.BS_PATH)
    result = build_blend_space.run({})
    assert any("BS_Locomotion" in f for f in result["failed"])

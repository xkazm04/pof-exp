"""AM_Roll montage build."""

import unreal

from player_movement import build_montage


def setup_function(_fn):
    unreal.EditorAssetLibrary._present.clear()
    unreal.EditorAssetLibrary._assets.clear()
    # Source retargeted clip exists
    unreal.EditorAssetLibrary._present.add(build_montage.SRC_PATH)
    unreal.EditorAssetLibrary._assets[build_montage.SRC_PATH] = object()


def test_creates_am_roll_on_first_run():
    result = build_montage.run({})
    assert "AM_Roll" in result["created"]


def test_skips_when_already_exists():
    target = f"{build_montage.PACKAGE}/{build_montage.ASSET_NAME}"
    unreal.EditorAssetLibrary._present.add(target)
    result = build_montage.run({})
    assert "AM_Roll" in result["skipped"]


def test_missing_source_clip_fails_cleanly():
    unreal.EditorAssetLibrary._present.discard(build_montage.SRC_PATH)
    result = build_montage.run({})
    assert any("missing source clip" in f for f in result["failed"])

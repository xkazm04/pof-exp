"""Idempotency + missing-file behavior for player_movement.import_clips."""

import sys
import unreal

from player_movement import import_clips


def setup_function(_fn):
    unreal.EditorAssetLibrary._present.clear()
    unreal.EditorAssetLibrary._assets.clear()


def test_skips_already_imported_clips(tmp_path):
    # Two source FBXs on disk
    (tmp_path / "Standard_Idle.fbx").write_bytes(b"x" * 50_000)
    (tmp_path / "Walking.fbx").write_bytes(b"x" * 50_000)

    # One already exists in the destination — should be skipped
    unreal.EditorAssetLibrary._present.add("/Game/Mixamo/Raw/Standard_Idle")

    result = import_clips.run({"raw_dir": str(tmp_path)})

    assert "Standard_Idle" in result["skipped"]
    assert "Walking" in result["created"]
    # The other 8 EXPECTED clips weren't in tmp_path → failed list
    assert len([f for f in result["failed"] if "missing source FBX" in f]) == 8


def test_missing_raw_dir_returns_failure():
    result = import_clips.run({"raw_dir": "/does/not/exist"})
    assert result["created"] == []
    assert result["skipped"] == []
    assert any("raw_dir not found" in f for f in result["failed"])


def test_no_raw_dir_arg_fails_cleanly():
    result = import_clips.run({})
    assert any("raw_dir" in f for f in result["failed"])

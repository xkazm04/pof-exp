"""Retarget skip + create behavior."""

import unreal
import types

from player_movement import retarget


def setup_function(_fn):
    unreal.EditorAssetLibrary._present.clear()
    unreal.EditorAssetLibrary._assets.clear()
    unreal.AssetRegistryHelpers._registry._path_to_assets.clear()
    # Retargeter present
    unreal.EditorAssetLibrary._present.add(retarget.RETARGETER_PATH)
    unreal.EditorAssetLibrary._assets[retarget.RETARGETER_PATH] = object()


def _seed_clips(*names):
    unreal.AssetRegistryHelpers._registry._path_to_assets[retarget.SRC_DIR] = [
        types.SimpleNamespace(
            object_path=f"{retarget.SRC_DIR}/{n}",
            asset_class="AnimSequence",
            asset_class_path=types.SimpleNamespace(asset_name="AnimSequence"),
        )
        for n in names
    ]


def test_retargets_unretargeted_clips():
    _seed_clips("Standard_Idle", "Walking")
    result = retarget.run({})
    # _RT outputs created
    rts = [n for n in result["created"] if n.endswith("_RT")]
    assert set(rts) == {"Standard_Idle_RT", "Walking_RT"}


def test_skips_already_retargeted():
    _seed_clips("Standard_Idle", "Walking")
    unreal.EditorAssetLibrary._present.add(f"{retarget.DST_DIR}/Standard_Idle_RT")
    result = retarget.run({})
    assert "Standard_Idle" in result["skipped"]


def test_missing_retargeter_fails_cleanly():
    unreal.EditorAssetLibrary._present.discard(retarget.RETARGETER_PATH)
    result = retarget.run({})
    assert any("retargeter missing" in f for f in result["failed"])

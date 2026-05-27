"""Smoke + idempotency for build_ik_rigs."""

import unreal

from player_movement import build_ik_rigs


def setup_function(_fn):
    unreal.EditorAssetLibrary._present.clear()
    unreal.EditorAssetLibrary._assets.clear()
    # The pipeline assumes step 3 ran first (Mixamo skeleton exists) and Manny is shipping.
    unreal.EditorAssetLibrary._present.add(build_ik_rigs.SKEL_MIXAMO)
    unreal.EditorAssetLibrary._assets[build_ik_rigs.SKEL_MIXAMO] = object()
    manny = build_ik_rigs.SKEL_MANNY_CANDIDATES[0]
    unreal.EditorAssetLibrary._present.add(manny)
    unreal.EditorAssetLibrary._assets[manny] = object()


def test_builds_three_assets_on_first_run():
    result = build_ik_rigs.run({})
    assert set(result["created"]) >= {"IK_Mixamo", "IK_Manny", "RTG_MixamoToManny"}
    assert result["failed"] == []


def test_skips_existing_on_second_run():
    build_ik_rigs.run({})           # first run creates everything
    result = build_ik_rigs.run({})  # second run sees everything already there
    assert set(result["skipped"]) == {"IK_Mixamo", "IK_Manny", "RTG_MixamoToManny"}
    assert result["created"] == []


def test_reports_missing_mixamo_skeleton():
    unreal.EditorAssetLibrary._present.discard(build_ik_rigs.SKEL_MIXAMO)
    result = build_ik_rigs.run({})
    assert any("Mixamo skeleton not yet imported" in f for f in result["failed"])

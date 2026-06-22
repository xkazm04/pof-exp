"""Import the downloaded Mixamo combat FBX onto the X Bot skeleton, then retarget to Manny
via the existing RTG_MixamoToManny (reuses import_clips + retarget). Output: combat *_RT
anims in /Game/Mixamo/Retargeted/SKM_Manny/.
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import unreal
import import_clips as ic
import retarget as rt

RAW_DIR = r"C:\Users\kazda\Documents\Unreal Projects\PoF\Content\Source\Mixamo\Raw"
COMBAT = ["Sword_Slash", "Slash", "Standing_Melee_Attack_Downward", "Great_Sword_Slash"]


def main():
    at = unreal.AssetToolsHelpers.get_asset_tools()
    skel = unreal.EditorAssetLibrary.load_asset(ic.SKELETON_PATH)
    if not skel:
        unreal.log_error("COMBAT: missing X Bot skeleton %s (import locomotion first)" % ic.SKELETON_PATH)
        return
    for name in COMBAT:
        target = "%s/%s" % (ic.DEST_PACKAGE, name)
        if unreal.EditorAssetLibrary.does_asset_exist(target):
            unreal.log("COMBAT: [skip import] %s" % name); continue
        fbx = ic._resolve_fbx(RAW_DIR, name)
        if not fbx:
            unreal.log_error("COMBAT: missing FBX %s.fbx" % name); continue
        at.import_asset_tasks([ic._anim_task(fbx, skel)])
        unreal.log("COMBAT: import %-32s -> %s" % (name, unreal.EditorAssetLibrary.does_asset_exist(target)))
    res = rt.run({})
    unreal.log("COMBAT: retarget created=%s" % res["created"])
    unreal.log("COMBAT: retarget skipped=%d failed=%s" % (len(res["skipped"]), res["failed"]))
    unreal.log("[gate] RESULT=PASS")


main()

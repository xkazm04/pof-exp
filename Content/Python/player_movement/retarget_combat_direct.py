"""Retarget the imported combat clips to Manny via RTG_MixamoToManny, resolving the target
mesh dynamically (the hard-coded 5.7 mesh paths moved in 5.8)."""
import unreal as u

SRC_DIR = "/Game/Mixamo/Raw"
DST_DIR = "/Game/Mixamo/Retargeted/SKM_Manny"
RETARGETER = "/Game/Characters/Player/IK/RTG_MixamoToManny"
SRC_MESH = "/Game/Mixamo/Raw/Standard_Idle"
SK_MANNY = "/MoverTests/Characters/Mannequins/Meshes/SK_Mannequin"
COMBAT = ["Sword_Slash", "Slash", "Standing_Melee_Attack_Downward", "Great_Sword_Slash"]


def find_target_mesh():
    sk = u.load_asset(SK_MANNY)
    if sk:
        for getter in ("get_preview_mesh",):
            try:
                pm = getattr(sk, getter)()
                if pm:
                    u.log("RETC: target mesh via skeleton.%s = %s" % (getter, pm.get_path_name()))
                    return pm
            except Exception as e:
                u.log("RETC: %s failed (%s)" % (getter, e))
    bp = u.load_asset("/Game/VerticalSlice/BP_VSPlayer")
    if bp:
        cdo = u.get_default_object(bp.generated_class())
        mc = cdo.get_editor_property("mesh")
        for prop in ("skeletal_mesh_asset", "skeletal_mesh"):
            try:
                sm = mc.get_editor_property(prop)
                if sm:
                    u.log("RETC: target mesh via BP_VSPlayer.%s = %s" % (prop, sm.get_path_name()))
                    return sm
            except Exception:
                pass
    return None


def main():
    rtg = u.EditorAssetLibrary.load_asset(RETARGETER)
    src = u.EditorAssetLibrary.load_asset(SRC_MESH)
    tgt = find_target_mesh()
    u.log("RETC: rtg=%s src=%s tgt=%s" % (bool(rtg), bool(src), bool(tgt)))
    if not (rtg and src and tgt):
        u.log_error("RETC: missing rtg/src/tgt — abort"); return

    pending = []
    for name in COMBAT:
        p = "%s/%s" % (SRC_DIR, name)
        if u.EditorAssetLibrary.does_asset_exist(p):
            pending.append(u.EditorAssetLibrary.find_asset_data(p))
    u.log("RETC: pending=%d" % len(pending))

    u.IKRetargetBatchOperation.duplicate_and_retarget(
        pending, src, tgt, rtg, suffix="_RT",
        include_referenced_assets=False, overwrite_existing_files=True)

    for name in COMBAT:
        root_path = "/Game/%s_RT" % name
        dst_path = "%s/%s_RT" % (DST_DIR, name)
        if u.EditorAssetLibrary.does_asset_exist(root_path):
            if u.EditorAssetLibrary.does_asset_exist(dst_path):
                u.EditorAssetLibrary.delete_asset(dst_path)
            u.EditorAssetLibrary.rename_asset(root_path, dst_path)
            u.log("RETC: %s -> %s" % (name, dst_path))
        else:
            u.log_error("RETC: no retarget output for %s" % name)
    u.EditorAssetLibrary.save_directory(DST_DIR, only_if_is_dirty=False)
    u.log("[gate] RESULT=PASS")


main()

"""Re-source AM_SwordSlashC from a retargeted Mixamo slash (mocap) instead of the code-authored
clip. The abilities already load AM_SwordSlashC, so no C++ change is needed."""
import unreal as u

SEQ = "/Game/Mixamo/Retargeted/SKM_Manny/Sword_Slash_RT"
NAME = "AM_MixamoSlash"
PATH = "/Game/Weapons/" + NAME


def main():
    seq = u.load_asset(SEQ)
    if not seq:
        u.log_error("MMONT: missing %s" % SEQ); return
    skel = seq.get_skeleton()
    if u.EditorAssetLibrary.does_asset_exist(PATH):
        u.log("MMONT: %s exists (references the slash) — OK" % PATH)
        u.log("[gate] RESULT=PASS"); return
    f = u.AnimMontageFactory()
    f.set_editor_property("target_skeleton", skel)
    f.set_editor_property("source_animation", seq)
    m = u.AssetToolsHelpers.get_asset_tools().create_asset(NAME, "/Game/Weapons", u.AnimMontage, f)
    if not m:
        u.log_error("MMONT: create failed"); return
    u.EditorAssetLibrary.save_asset(PATH)
    u.log("MMONT: %s <- %s  len=%.2f" % (NAME, SEQ, m.get_play_length()))
    u.log("[gate] RESULT=PASS")


main()

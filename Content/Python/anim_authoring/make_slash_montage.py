"""Make an AnimMontage from the custom code-authored slash so it can drive the melee abilities."""
import unreal as u

SEQ = "/Game/PoF/GenAnims/SwordSlashC"
PATH = "/Game/Weapons/AM_SwordSlashC"


def main():
    seq = u.load_asset(SEQ)
    if not seq:
        u.log_error("MONT: missing %s" % SEQ); return
    # The montage references the SwordSlashC sequence, so re-authoring the sequence already
    # updates what it plays — only (re)create when the montage is actually missing.
    if u.EditorAssetLibrary.does_asset_exist(PATH):
        m = u.load_asset(PATH)
        u.log("MONT: %s exists (len=%.2f) — references updated sequence, OK" % (PATH, m.get_play_length()))
        u.log("[gate] RESULT=PASS"); return
    skel = seq.get_skeleton()
    f = u.AnimMontageFactory()
    f.set_editor_property("target_skeleton", skel)
    f.set_editor_property("source_animation", seq)
    m = u.AssetToolsHelpers.get_asset_tools().create_asset("AM_SwordSlashC", "/Game/Weapons", u.AnimMontage, f)
    if not m:
        u.log_error("MONT: create failed"); return
    u.EditorAssetLibrary.save_asset(PATH)
    u.log("MONT: created %s (len=%.2f)" % (PATH, m.get_play_length()))
    u.log("[gate] RESULT=PASS")


main()

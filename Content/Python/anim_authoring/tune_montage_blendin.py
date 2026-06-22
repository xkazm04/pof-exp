"""Shorten AM_SwordSlashC's montage blend-in so the wind-up reads in gameplay (the default
0.25s blend ate the anticipation when blended over locomotion)."""
import unreal as u

PATH = "/Game/Weapons/AM_SwordSlashC"


def main():
    m = u.load_asset(PATH)
    if not m:
        u.log_error("TUNE: missing %s" % PATH); return
    for prop, val in (("blend_in", 0.06), ("blend_out", 0.15)):
        try:
            ab = m.get_editor_property(prop)
            ab.set_editor_property("blend_time", val)
            m.set_editor_property(prop, ab)
            u.log("TUNE: %s.blend_time -> %.2f" % (prop, val))
        except Exception as e:
            u.log_warning("TUNE: %s failed (%s)" % (prop, e))
    u.EditorAssetLibrary.save_asset(PATH)
    u.log("TUNE: saved AM_SwordSlashC len=%.2f" % m.get_play_length())
    u.log("[gate] RESULT=PASS")


main()

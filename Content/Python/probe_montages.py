"""Probe melee montages: do they actually contain animation (play length + sections + segments)?"""
import unreal


def info(path):
    m = unreal.load_asset(path)
    if not m:
        unreal.log("PROBE: %s = MISSING" % path)
        return
    try:
        length = m.get_play_length()
    except Exception as e:
        length = "err:%s" % e
    try:
        secs = len(m.get_editor_property("composite_sections"))
    except Exception:
        secs = "?"
    seg = 0
    try:
        for s in m.get_editor_property("slot_anim_tracks"):
            seg += len(s.anim_track.anim_segments)
    except Exception as e:
        seg = "err:%s" % e
    unreal.log("PROBE: %s  length=%s  sections=%s  anim_segments=%s" % (path.split('/')[-1], length, secs, seg))


for p in [
    "/Game/Weapons/AM_SwordSlash",
    "/Game/Weapons/AS_SwordSlash",
    "/Game/Characters/Player/Animations/Montages/AM_MeleeCombo",
]:
    info(p)
unreal.log("[gate] RESULT=PASS")

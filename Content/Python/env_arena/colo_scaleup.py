"""Colosseum — scale the BATTLEGROUND to real-colosseum size (4x linear, ~80 m floor).

Scales the arena mesh + ground plane out, repositions/enlarges the ruins to populate the
bigger floor, and (importantly) DISABLES the hero camera's auto-activate so the level is
playable with the normal gameplay camera (the hero cam stays for offscreen captures only).
"""
import unreal

MAP = "/Game/Maps/Arena_Ancient"
S = 4.0


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    n_ruin = 0
    for a in list(eas.get_all_level_actors()):
        lbl = a.get_actor_label()
        if lbl == "Arena":
            a.set_actor_scale3d(unreal.Vector(S, S, 2.0))           # 4x floor, 2x wall height
        elif lbl.startswith("Plane"):
            sc = a.get_actor_scale3d()
            a.set_actor_scale3d(unreal.Vector(sc.x * 4.0, sc.y * 4.0, 1.0))  # huge sand ground
            loc = a.get_actor_location()
            a.set_actor_location(unreal.Vector(loc.x, loc.y, -2.0), False, False)
        elif lbl.startswith("Ruin_") or lbl.startswith("Rubble_"):
            loc = a.get_actor_location()
            a.set_actor_location(unreal.Vector(loc.x * S, loc.y * S, loc.z * 2.4), False, False)
            sc = a.get_actor_scale3d()
            a.set_actor_scale3d(unreal.Vector(sc.x * 2.4, sc.y * 2.4, sc.z * 2.4))
            n_ruin += 1
        elif lbl == "ColoCam_Hero":
            a.set_editor_property("auto_activate_for_player", unreal.AutoReceiveInput.DISABLED)
        elif lbl == "Arena_NavBounds":
            sc = a.get_actor_scale3d()
            a.set_actor_scale3d(unreal.Vector(sc.x * S, sc.y * S, sc.z))

    saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("[SCALEUP] arena x%.0f, ruins=%d rescaled, hero-cam auto-activate OFF, saved=%s"
               % (S, n_ruin, saved))
    unreal.log("[gate] RESULT=%s" % ("PASS" if saved else "FAIL"))


main()

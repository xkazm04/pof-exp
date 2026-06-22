"""
Build the Star Wars duel map for the Force Push knockback gate (crash-proof).

The editor is launched with /Game/Maps/VSEnemyAttack as the STARTUP map (no runtime
load_level — that crashes headless/-nullrhi). VSEnemyAttack already has a
player-spawning GameMode + an AARPGEnemyCharacter. This strips its functional-test
actor, drops in AVSForcePushKnockbackTest, and SAVE-AS to /Game/Maps/VSForcePush
(VSEnemyAttack itself is never saved, so it stays clean).

Run headless (editor idles — poll the abslog marker, then kill):
  UnrealEditor-Cmd PoF.uproject /Game/Maps/VSEnemyAttack -ExecCmds="py import fp_make_duel_map" -nullrhi ...
"""
import unreal

DST = "/Game/Maps/VSForcePush"


def main():
    unreal.log("FPSETUP: BEGIN")
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = ues.get_editor_world()
    unreal.log("FPSETUP: editor world = %s" % (world.get_name() if world else "None"))

    removed = 0
    for a in eas.get_all_level_actors():
        if isinstance(a, unreal.FunctionalTest):
            unreal.log("FPSETUP: removing existing test '%s'" % a.get_actor_label())
            eas.destroy_actor(a)
            removed += 1
    unreal.log("FPSETUP: removed %d existing test actor(s)" % removed)

    spawned = eas.spawn_actor_from_class(
        unreal.VSForcePushKnockbackTest, unreal.Vector(0.0, 0.0, 100.0)
    )
    if not spawned:
        unreal.log_error("FPSETUP: failed to spawn AVSForcePushKnockbackTest")
        unreal.log("[gate] RESULT=FAIL")
        return
    spawned.set_actor_label("ForcePushKnockback")
    unreal.log("FPSETUP: spawned test actor '%s'" % spawned.get_actor_label())

    ok = unreal.EditorLoadingAndSavingUtils.save_map(world, DST)
    unreal.log("FPSETUP: save_map(%s) -> %s" % (DST, ok))
    unreal.log("[gate] RESULT=%s" % ("PASS" if ok else "FAIL"))


main()

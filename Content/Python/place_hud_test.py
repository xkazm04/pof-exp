import unreal

MAP_PATH = "/Game/Maps/VerticalSlice"

unreal.EditorLoadingAndSavingUtils.load_map(MAP_PATH)

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
already = [a for a in actor_subsystem.get_all_level_actors()
           if a.get_class().get_name() == "VSHUDFunctionalTest"]

if already:
    unreal.log("place_hud_test: VSHUDFunctionalTest already present - nothing to do")
else:
    actor = actor_subsystem.spawn_actor_from_class(
        unreal.VSHUDFunctionalTest, unreal.Vector(0.0, 0.0, 200.0))
    if actor:
        actor.set_actor_label("VSHUDFunctionalTest")
        unreal.EditorLoadingAndSavingUtils.save_current_level()
        unreal.log("place_hud_test: spawned VSHUDFunctionalTest and saved the level")
    else:
        unreal.log_error("place_hud_test: spawn FAILED")

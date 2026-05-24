"""Idempotently place the 3 arena functional-test actors into VerticalSlice."""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/Maps/VerticalSlice")
aes = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

specs = [
    (unreal.VSArenaCollisionTest, "VSArenaCollisionTest"),
    (unreal.VSArenaBoundsTest, "VSArenaBoundsTest"),
    (unreal.VSArenaSetupTest, "VSArenaSetupTest"),
    (unreal.VSFootstepWiringTest, "VSFootstepWiringTest"),
]
classes = tuple(c for c, _ in specs)
for a in aes.get_all_level_actors():
    if isinstance(a, classes):
        aes.destroy_actor(a)
for cls, label in specs:
    t = aes.spawn_actor_from_class(cls, unreal.Vector(0.0, 0.0, 400.0))
    t.set_actor_label(label)
    unreal.log("[place_arena_tests] placed " + label)

les.save_current_level()
unreal.log("[place_arena_tests] done")

if __name__ == "__main__":
    try:
        if unreal.is_editor():
            unreal.SystemLibrary.quit_editor()
    except Exception:
        pass

"""
place_material_tests.py
=======================
Idempotently place the material functional-test actors into VerticalSlice
(folder-06 tests.md UE #1/#3). Mirrors place_arena_tests.py.

Prereq: the C++ classes AVSArenaMaterialBindingTest + AVSMasterMaterialInstanceTest
must be compiled first (the Python binding `unreal.VS...Test` only exists after a
successful build of the PoF module).

NOTE: this edits the shared VerticalSlice.umap — run it deliberately, not
concurrently with another session editing the same map.
"""
import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level("/Game/Maps/VerticalSlice")
aes = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

specs = [
    (unreal.VSArenaMaterialBindingTest, "VSArenaMaterialBindingTest"),
    (unreal.VSMasterMaterialInstanceTest, "VSMasterMaterialInstanceTest"),
]
classes = tuple(c for c, _ in specs)

# Remove any prior copies (idempotent re-placement).
for a in aes.get_all_level_actors():
    if isinstance(a, classes):
        aes.destroy_actor(a)

for cls, label in specs:
    t = aes.spawn_actor_from_class(cls, unreal.Vector(0.0, 0.0, 450.0))
    t.set_actor_label(label)
    unreal.log("[place_material_tests] placed " + label)

les.save_current_level()
unreal.log("[place_material_tests] done")

if __name__ == "__main__":
    try:
        if unreal.is_editor():
            unreal.SystemLibrary.quit_editor()
    except Exception:
        pass

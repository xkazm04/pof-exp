"""
place_combat_tests.py
=====================
Idempotent: places the Combat-1 functional-test actors in the VerticalSlice map
and saves. Run headless (after the C++ classes are compiled):

    "C:\\Program Files\\Epic Games\\UE_5.7\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
        "C:\\Users\\kazda\\Documents\\Unreal Projects\\PoF\\PoF.uproject" ^
        -run=pythonscript -script="<abs path to this file>" -unattended -nopause
"""

import unreal

LEVEL_PATH = "/Game/Maps/VerticalSlice"

# (class /Script path, actor label, spawn location)
TESTS = [
    ("/Script/PoF.VSCombatGrayBoxPathTest", "VSCombatGrayBoxPathTest", unreal.Vector(0.0, 0.0, 220.0)),
    ("/Script/PoF.VSCombatAbilityGrantTest", "VSCombatAbilityGrantTest", unreal.Vector(0.0, 0.0, 240.0)),
]


def main():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    aes = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if not les.load_level(LEVEL_PATH):
        raise RuntimeError("Failed to load level " + LEVEL_PATH)

    existing = {a.get_actor_label() for a in aes.get_all_level_actors()}

    for cls_path, label, loc in TESTS:
        if label in existing:
            unreal.log("[place_combat_tests] already present, skipping: " + label)
            continue
        cls = unreal.load_class(None, cls_path)
        if cls is None:
            raise RuntimeError("Could not load class " + cls_path)
        actor = aes.spawn_actor_from_class(cls, loc)
        actor.set_actor_label(label)
        unreal.log("[place_combat_tests] placed: " + label)

    les.save_current_level()
    unreal.log("[place_combat_tests] saved level " + LEVEL_PATH)


main()

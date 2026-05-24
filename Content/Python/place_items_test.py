"""
place_items_test.py
===================
Builds the items catalog gate test map.

/Game/Maps/VSItems: floor + lights + PlayerStart + an AVSItemsDefinitionsTest
actor. The test is self-contained (instantiates a transient UARPGItemDefinition
and asserts the schema in StartTest); no enemy/loot/player setup is needed.

Idempotent: re-running overwrites the level. Run headless:
    "C:\\Program Files\\Epic Games\\UE_5.7\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
        "C:\\Users\\kazda\\Documents\\Unreal Projects\\PoF\\PoF.uproject" ^
        -run=pythonscript -script="<abs path to this file>" -unattended -nopause
"""

import unreal

TEST_LEVEL_PATH = "/Game/Maps/VSItems"
CLS_ITEMS_TEST = "/Script/PoF.VSItemsDefinitionsTest"
CUBE_MESH = "/Engine/BasicShapes/Cube"

asset_lib = unreal.EditorAssetLibrary
_PROGRESS = []


def _log(msg):
    unreal.log_warning("[place_items_test] " + msg)
    _PROGRESS.append(msg)


def load_class(class_path):
    cls = unreal.load_class(None, class_path)
    if cls is None:
        cls = unreal.load_class(None, class_path + "_C")
    return cls


def build_test_level():
    try:
        _log("--- Building test level %s ---" % TEST_LEVEL_PATH)
        level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

        if asset_lib.does_asset_exist(TEST_LEVEL_PATH):
            level_subsystem.new_level("/Temp/_vsitems_scratch")
            asset_lib.delete_asset(TEST_LEVEL_PATH)
            _log("Deleted prior test level for clean rebuild")

        if not level_subsystem.new_level(TEST_LEVEL_PATH):
            raise RuntimeError("new_level failed for " + TEST_LEVEL_PATH)

        cube_mesh = asset_lib.load_asset(CUBE_MESH)
        if cube_mesh is None:
            raise RuntimeError("Cube mesh missing: " + CUBE_MESH)

        # Minimal scaffold — floor + lights + PlayerStart (so the engine has a pawn).
        floor = actor_subsystem.spawn_actor_from_class(
            unreal.StaticMeshActor, unreal.Vector(0.0, 0.0, 0.0))
        floor.set_actor_label("Floor")
        floor.set_actor_scale3d(unreal.Vector(20.0, 20.0, 1.0))
        floor.static_mesh_component.set_editor_property("static_mesh", cube_mesh)
        floor.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)

        actor_subsystem.spawn_actor_from_class(
            unreal.DirectionalLight, unreal.Vector(0.0, 0.0, 1000.0),
            unreal.Rotator(-45.0, 0.0, 0.0)).set_actor_label("DirectionalLight")
        actor_subsystem.spawn_actor_from_class(
            unreal.SkyLight, unreal.Vector(0.0, 0.0, 1000.0)).set_actor_label("SkyLight")
        actor_subsystem.spawn_actor_from_class(
            unreal.PlayerStart, unreal.Vector(0.0, 0.0, 150.0)).set_actor_label("PlayerStart")

        # Functional test actor.
        test_cls = load_class(CLS_ITEMS_TEST)
        if test_cls is None:
            raise RuntimeError("Could not load " + CLS_ITEMS_TEST + " (rebuild the editor?)")
        actor_subsystem.spawn_actor_from_class(
            test_cls, unreal.Vector(0.0, 0.0, 200.0)).set_actor_label("VSItemsDefinitionsTest")

        level_subsystem.save_current_level()
        _log("Saved test level: " + TEST_LEVEL_PATH)
    except Exception as exc:
        unreal.log_error("[place_items_test] build_test_level FAILED: " + str(exc))
        raise


def main():
    _log("=== VSItems test map build START ===")
    build_test_level()
    _log("=== VSItems test map build COMPLETE ===")
    try:
        out_path = unreal.Paths.combine(
            [unreal.Paths.project_saved_dir(), "place_items_test.log"])
        with open(out_path, "w", encoding="utf-8") as fh:
            fh.write("\n".join(_PROGRESS) + "\n")
        unreal.log_warning("[place_items_test] progress written to " + out_path)
    except Exception as exc:
        unreal.log_error("[place_items_test] could not write progress log: " + str(exc))


if __name__ == "__main__":
    main()

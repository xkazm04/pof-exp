"""
place_spellbook_test.py
=======================
Builds the isolated functional-test map for the folder-09 generation proof.

/Game/Maps/VS09Ability: floor, lights, PlayerStart, a placed BP_VSEnemy (the
damage target), and an AVSAbility09Test actor. The project's GlobalDefaultGameMode
spawns BP_VSPlayer (the caster). The test grants UGA_VS09Smite to the player at
runtime and asserts the enemy's Health drops — so no Blueprint/CDO wiring is
needed here.

Idempotent: re-running overwrites the level. Run headless:
    "C:\\Program Files\\Epic Games\\UE_5.7\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
        "C:\\Users\\kazda\\Documents\\Unreal Projects\\PoF\\PoF.uproject" ^
        -run=pythonscript -script="<abs path to this file>" -unattended -nopause
"""

import unreal

BP_VSENEMY_PATH = "/Game/VerticalSlice/BP_VSEnemy"
TEST_LEVEL_PATH = "/Game/Maps/VS09Ability"
CLS_ABILITY_TEST = "/Script/PoF.VSAbility09Test"
CUBE_MESH = "/Engine/BasicShapes/Cube"

asset_lib = unreal.EditorAssetLibrary
_PROGRESS = []


def _log(msg):
    unreal.log_warning("[place_spellbook_test] " + msg)
    _PROGRESS.append(msg)


def load_class(class_path):
    cls = unreal.load_class(None, class_path)
    if cls is None:
        cls = unreal.load_class(None, class_path + "_C")
    return cls


def load_object(object_path):
    if not asset_lib.does_asset_exist(object_path):
        return None
    return asset_lib.load_asset(object_path)


def build_test_level():
    try:
        _log("--- Building test level %s ---" % TEST_LEVEL_PATH)
        level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

        if asset_lib.does_asset_exist(TEST_LEVEL_PATH):
            level_subsystem.new_level("/Temp/_vs09_scratch")
            asset_lib.delete_asset(TEST_LEVEL_PATH)
            _log("Deleted prior test level for clean rebuild")

        if not level_subsystem.new_level(TEST_LEVEL_PATH):
            raise RuntimeError("new_level failed for " + TEST_LEVEL_PATH)

        cube_mesh = load_object(CUBE_MESH)
        if cube_mesh is None:
            raise RuntimeError("Cube mesh missing: " + CUBE_MESH)

        # Floor
        floor = actor_subsystem.spawn_actor_from_class(
            unreal.StaticMeshActor, unreal.Vector(0.0, 0.0, 0.0))
        floor.set_actor_label("Floor")
        floor.set_actor_scale3d(unreal.Vector(40.0, 40.0, 1.0))
        floor_smc = floor.static_mesh_component
        floor_smc.set_editor_property("static_mesh", cube_mesh)
        floor_smc.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
        floor_smc.set_mobility(unreal.ComponentMobility.STATIC)

        # Lights
        actor_subsystem.spawn_actor_from_class(
            unreal.DirectionalLight, unreal.Vector(0.0, 0.0, 1000.0),
            unreal.Rotator(-45.0, 0.0, 0.0)).set_actor_label("DirectionalLight")
        actor_subsystem.spawn_actor_from_class(
            unreal.SkyLight, unreal.Vector(0.0, 0.0, 1000.0)).set_actor_label("SkyLight")

        # Player start
        actor_subsystem.spawn_actor_from_class(
            unreal.PlayerStart, unreal.Vector(0.0, 0.0, 150.0)).set_actor_label("PlayerStart")

        # Enemy target (BP_VSEnemy) — placed ~300uu from the player start.
        bp_enemy = load_object(BP_VSENEMY_PATH)
        enemy_cls = bp_enemy.generated_class() if bp_enemy else None
        if enemy_cls is None:
            raise RuntimeError("BP_VSEnemy has no generated class: " + BP_VSENEMY_PATH)
        actor_subsystem.spawn_actor_from_class(
            enemy_cls, unreal.Vector(300.0, 0.0, 150.0)).set_actor_label("VSEnemy")

        # Functional test actor
        test_cls = load_class(CLS_ABILITY_TEST)
        if test_cls is None:
            raise RuntimeError("Could not load " + CLS_ABILITY_TEST + " (rebuild the editor?)")
        actor_subsystem.spawn_actor_from_class(
            test_cls, unreal.Vector(0.0, 0.0, 200.0)).set_actor_label("VSAbility09Test")

        level_subsystem.save_current_level()
        _log("Saved test level: " + TEST_LEVEL_PATH)
    except Exception as exc:
        unreal.log_error("[place_spellbook_test] build_test_level FAILED: " + str(exc))
        raise


def main():
    _log("=== VS09 ability test map build START ===")
    build_test_level()
    _log("=== VS09 ability test map build COMPLETE ===")
    try:
        out_path = unreal.Paths.combine(
            [unreal.Paths.project_saved_dir(), "place_spellbook_test.log"])
        with open(out_path, "w", encoding="utf-8") as fh:
            fh.write("\n".join(_PROGRESS) + "\n")
        unreal.log_warning("[place_spellbook_test] progress written to " + out_path)
    except Exception as exc:
        unreal.log_error("[place_spellbook_test] could not write progress log: " + str(exc))


if __name__ == "__main__":
    main()

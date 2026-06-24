"""
place_inventory_potion_test.py
==============================
Builds /Game/Maps/VSInvPotion: a minimal self-contained map (floor + lights +
PlayerStart) hosting an AVSInventoryPotionTest functional-test actor. The test
loads /Game/Inventory/DA_HealthPotion and asserts it is a Consumable whose
OnUseEffect (UGE_HealthPotion) heals +50 — the config gate behind the runtime
loot->heal proof.

Run the functional test headless (judge by the abslog Result={Success} marker):
    UnrealEditor-Cmd.exe "<uproject>" ^
        -ExecCmds="Automation RunTests Project.Functional Tests./Game/Maps/VSInvPotion;Quit" ^
        -unattended -nopause -nosplash -nullrhi -log -abslog="<log>"

Idempotent: re-running overwrites the level. Build it headless:
    UnrealEditor-Cmd.exe "<uproject>" -run=pythonscript -script="<abs path>" -unattended -nopause
"""

import unreal

TEST_LEVEL_PATH = "/Game/Maps/VSInvPotion"
CLS_INV_TEST = "/Script/PoF.VSInventoryPotionTest"
CUBE_MESH = "/Engine/BasicShapes/Cube"

asset_lib = unreal.EditorAssetLibrary
_PROGRESS = []


def _log(msg):
    unreal.log_warning("[place_inv_test] " + msg)
    _PROGRESS.append(msg)


def load_class(class_path):
    cls = unreal.load_class(None, class_path)
    if cls is None:
        cls = unreal.load_class(None, class_path + "_C")
    return cls


def build():
    level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if asset_lib.does_asset_exist(TEST_LEVEL_PATH):
        level_sub.new_level("/Temp/_vsinvpotion_scratch")
        asset_lib.delete_asset(TEST_LEVEL_PATH)
        _log("Deleted prior test level for clean rebuild")

    if not level_sub.new_level(TEST_LEVEL_PATH):
        raise RuntimeError("new_level failed for " + TEST_LEVEL_PATH)

    cube_mesh = asset_lib.load_asset(CUBE_MESH)
    if cube_mesh is None:
        raise RuntimeError("Cube mesh missing: " + CUBE_MESH)

    floor = actor_sub.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0.0, 0.0, 0.0))
    floor.set_actor_label("Floor")
    floor.set_actor_scale3d(unreal.Vector(20.0, 20.0, 1.0))
    floor.static_mesh_component.set_editor_property("static_mesh", cube_mesh)
    floor.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)

    actor_sub.spawn_actor_from_class(
        unreal.DirectionalLight, unreal.Vector(0.0, 0.0, 1000.0),
        unreal.Rotator(-45.0, 0.0, 0.0)).set_actor_label("DirectionalLight")
    actor_sub.spawn_actor_from_class(
        unreal.SkyLight, unreal.Vector(0.0, 0.0, 1000.0)).set_actor_label("SkyLight")
    actor_sub.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0.0, 0.0, 150.0)).set_actor_label("PlayerStart")

    test_cls = load_class(CLS_INV_TEST)
    if test_cls is None:
        raise RuntimeError("Could not load " + CLS_INV_TEST + " (rebuild the editor?)")
    actor_sub.spawn_actor_from_class(
        test_cls, unreal.Vector(0.0, 0.0, 200.0)).set_actor_label("VSInventoryPotionTest")

    level_sub.save_current_level()
    _log("Saved test level: " + TEST_LEVEL_PATH)


def main():
    _log("=== VSInvPotion test map build START ===")
    build()
    _log("=== VSInvPotion test map build COMPLETE ===")
    try:
        out_path = unreal.Paths.combine(
            [unreal.Paths.project_saved_dir(), "place_inventory_potion_test.log"])
        with open(out_path, "w", encoding="utf-8") as fh:
            fh.write("\n".join(_PROGRESS) + "\n")
        unreal.log_warning("[place_inv_test] progress written to " + out_path)
    except Exception as exc:  # noqa: BLE001
        unreal.log_error("[place_inv_test] could not write progress log: " + str(exc))


if __name__ == "__main__":
    main()

"""
setup_enemy_ai.py
=================
Makes the vertical-slice enemy hostile and builds an isolated functional-test map.

1. BP_VSEnemy CDO:
     - AIControllerClass -> AARPGSimpleAIController (pure-C++ chase+attack controller)
     - GrantedAbilities  += UGA_EnemyMeleeAttack (raw C++ class; DamageEffect defaults
       to GE_Damage in C++, so no config-BP is needed)
   (AutoPossessAI is already PlacedInWorldOrSpawned on the C++ class.)
2. /Game/Maps/VSEnemyAttack: floor, lights, PlayerStart, a placed BP_VSEnemy, and an
   AVSEnemyAttackTest actor. The project's GlobalDefaultGameMode spawns BP_VSPlayer.

Idempotent: re-running reuses/overwrites. Run headless:
    "C:\\Program Files\\Epic Games\\UE_5.7\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
        "C:\\Users\\kazda\\Documents\\Unreal Projects\\PoF\\PoF.uproject" ^
        -run=pythonscript -script="<abs path to this file>" -unattended -nopause
"""

import unreal

# --- Constants -------------------------------------------------------------
BP_VSENEMY_PATH = "/Game/VerticalSlice/BP_VSEnemy"
TEST_LEVEL_PATH = "/Game/Maps/VSEnemyAttack"

CLS_SIMPLE_AI = "/Script/PoF.ARPGSimpleAIController"
CLS_GA_ENEMY_MELEE = "/Script/PoF.GA_EnemyMeleeAttack"
CLS_ENEMY_ATTACK_TEST = "/Script/PoF.VSEnemyAttackTest"

CUBE_MESH = "/Engine/BasicShapes/Cube"

asset_lib = unreal.EditorAssetLibrary
_PROGRESS = []


def _log(msg):
    unreal.log_warning("[setup_enemy_ai] " + msg)
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


def get_cdo(bp):
    gen = bp.generated_class()
    if gen is None:
        raise RuntimeError("Blueprint has no generated class: " + bp.get_name())
    return unreal.get_default_object(gen)


# --- Section 1: wire BP_VSEnemy CDO ---------------------------------------

def wire_enemy_blueprint():
    try:
        _log("--- Wiring BP_VSEnemy ---")
        bp = load_object(BP_VSENEMY_PATH)
        if bp is None:
            raise RuntimeError("BP_VSEnemy not found: " + BP_VSENEMY_PATH)

        simple_ai = load_class(CLS_SIMPLE_AI)
        if simple_ai is None:
            raise RuntimeError("Could not load " + CLS_SIMPLE_AI + " (rebuild the editor?)")

        ga_melee = load_class(CLS_GA_ENEMY_MELEE)
        if ga_melee is None:
            raise RuntimeError("Could not load " + CLS_GA_ENEMY_MELEE)

        # Ensure the generated class is current before editing the CDO.
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        cdo = get_cdo(bp)

        cdo.set_editor_property("AIControllerClass", simple_ai)
        _log("BP_VSEnemy: AIControllerClass -> ARPGSimpleAIController")

        granted = cdo.get_editor_property("GrantedAbilities")
        granted = list(granted) if granted else []
        if ga_melee not in granted:
            granted.append(ga_melee)
        cdo.set_editor_property("GrantedAbilities", granted)
        _log("BP_VSEnemy: GrantedAbilities -> %d entry(ies)" % len(granted))

        asset_lib.save_asset(BP_VSENEMY_PATH)
        _log("BP_VSEnemy: saved")
    except Exception as exc:
        unreal.log_error("[setup_enemy_ai] wire_enemy_blueprint FAILED: " + str(exc))
        raise


# --- Section 2: build the isolated test map -------------------------------

def build_test_level():
    try:
        _log("--- Building test level %s ---" % TEST_LEVEL_PATH)
        level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

        # Fresh level (idempotent): switch off the target level before deleting it.
        if asset_lib.does_asset_exist(TEST_LEVEL_PATH):
            level_subsystem.new_level("/Temp/_enemyai_scratch")
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

        # Enemy (wired BP_VSEnemy) — within ~300uu so it's near the player.
        bp_enemy = load_object(BP_VSENEMY_PATH)
        enemy_cls = bp_enemy.generated_class() if bp_enemy else None
        if enemy_cls is None:
            raise RuntimeError("BP_VSEnemy has no generated class")
        enemy_actor = actor_subsystem.spawn_actor_from_class(
            enemy_cls, unreal.Vector(300.0, 0.0, 150.0))
        enemy_actor.set_actor_label("VSEnemy")

        # Force the controller class on the placed INSTANCE. Within a single
        # Python session the spawn template can be stale, baking the native
        # default (AARPGAIController) into the .umap and overriding the CDO at
        # PIE load. Setting it on the instance serialises the correct value.
        simple_ai = load_class(CLS_SIMPLE_AI)
        if simple_ai is None:
            raise RuntimeError("Could not load " + CLS_SIMPLE_AI)
        enemy_actor.set_editor_property("ai_controller_class", simple_ai)
        _log("VSEnemy instance: ai_controller_class -> ARPGSimpleAIController")

        # Functional test actor
        test_cls = load_class(CLS_ENEMY_ATTACK_TEST)
        if test_cls is None:
            raise RuntimeError("Could not load " + CLS_ENEMY_ATTACK_TEST)
        actor_subsystem.spawn_actor_from_class(
            test_cls, unreal.Vector(0.0, 0.0, 200.0)).set_actor_label("VSEnemyAttackTest")

        level_subsystem.save_current_level()
        _log("Saved test level: " + TEST_LEVEL_PATH)
    except Exception as exc:
        unreal.log_error("[setup_enemy_ai] build_test_level FAILED: " + str(exc))
        raise


def main():
    _log("=== enemy AI wiring START ===")
    wire_enemy_blueprint()
    build_test_level()
    _log("=== enemy AI wiring COMPLETE ===")

    try:
        out_path = unreal.Paths.combine(
            [unreal.Paths.project_saved_dir(), "setup_enemy_ai.log"])
        with open(out_path, "w", encoding="utf-8") as fh:
            fh.write("\n".join(_PROGRESS) + "\n")
        unreal.log_warning("[setup_enemy_ai] progress written to " + out_path)
    except Exception as exc:
        unreal.log_error("[setup_enemy_ai] could not write progress log: " + str(exc))


if __name__ == "__main__":
    main()

"""Step 10 helper — Build TestLevel_PlayerMovement.umap for the L4 gate.

Creates a small flat plane + a single PlayerStart at the origin. Idempotent:
re-runs are a no-op if the map exists.
"""

import unreal


LEVEL_PATH = "/Game/Maps/TestLevel_PlayerMovement"


def run(args):
    result = {"created": [], "skipped": [], "failed": []}
    try:
        if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
            result["skipped"].append(LEVEL_PATH)
            return result

        # Ensure parent dir exists
        unreal.EditorAssetLibrary.make_directory("/Game/Maps")

        # Create the new level (the EditorLevelLibrary handles current-level switching)
        unreal.EditorLevelLibrary.new_level(LEVEL_PATH)

        # Spawn a large flat floor
        floor = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.StaticMeshActor, unreal.Vector(0, 0, 0)
        )
        sm = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Plane")
        if floor and sm:
            floor.static_mesh_component.set_static_mesh(sm)
            floor.set_actor_scale3d(unreal.Vector(50, 50, 1))

        # PlayerStart above the floor
        unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.PlayerStart, unreal.Vector(0, 0, 100)
        )

        unreal.EditorLevelLibrary.save_current_level()
        result["created"].append(LEVEL_PATH)
    except Exception as e:
        result["failed"].append(str(e))
    return result

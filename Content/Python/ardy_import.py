"""Import ARDY-generated skinned FBX clips as SkeletalMesh + Skeleton + AnimSequence.

Run headless (proven commandlet recipe):
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=Content/Python/ardy_import.py -nullrhi
"""
import os

import unreal

SRC = r"C:\Users\kazda\kiro\ardy\outputs"
DEST = "/Game/Generated/Ardy"
CLIPS = ["slash", "run", "roll", "idle"]

# UE 5.8 routes FBX through Interchange, which ignores FbxImportUI in the
# pythonscript commandlet ("nothing to import") — force the legacy FBX path.
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")


def import_clip(name: str) -> None:
    fbx = os.path.join(SRC, f"{name}.fbx")
    ui = unreal.FbxImportUI()
    ui.import_mesh = True
    ui.import_as_skeletal = True
    ui.import_animations = True
    ui.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    ui.skeletal_mesh_import_data.set_editor_property("import_morph_targets", False)
    ui.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)

    task = unreal.AssetImportTask()
    task.filename = fbx
    task.destination_path = f"{DEST}/{name}"
    task.automated = True
    task.save = True
    task.options = ui

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    imported = list(task.imported_object_paths)
    print(f"POF_ARDY_IMPORT {name}: {imported}")


for clip in CLIPS:
    import_clip(clip)

# verify AnimSequences exist + report length
for clip in CLIPS:
    found = []
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = registry.get_assets_by_path(f"{DEST}/{clip}", recursive=True)
    for a in assets:
        if a.asset_class_path.asset_name == "AnimSequence":
            seq = a.get_asset()
            found.append(f"{a.asset_name} ({seq.get_editor_property('sequence_length'):.2f}s)")
    print(f"POF_ARDY_VERIFY {clip}: {found if found else 'NO AnimSequence'}")

print("POF_ARDY_DONE")

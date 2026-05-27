"""Step 03 — Batch FBX import from Content/Source/Mixamo/Raw/ to /Game/Mixamo/Raw/.

Idempotent: clips that already exist in the destination are skipped. The first
clip (Standard_Idle) is the rig-bearing import; subsequent clips re-use the
skeleton it brings.
"""

import os

import unreal


DEST_PACKAGE = "/Game/Mixamo/Raw"

# Order matters: Standard_Idle MUST be first so it brings the X Bot rig.
EXPECTED_CLIPS = [
    "Standard_Idle",
    "Walking",
    "Walking_Backwards",
    "Left_Strafe_Walking",
    "Right_Strafe_Walking",
    "Running",
    "Running_Backward",
    "Left_Strafe",
    "Right_Strafe",
    "Forward_Roll",
]


def _build_import_task(fbx_path, with_skin):
    """Build an AssetImportTask for a single Mixamo FBX clip."""
    task = unreal.AssetImportTask()
    task.filename = fbx_path
    task.destination_path = DEST_PACKAGE
    task.replace_existing = True
    task.automated = True
    task.save = True

    # Mixamo-specific FBX options (set what's available on the current UE Python build)
    try:
        opts = unreal.FbxImportUI()
        opts.import_animations = True
        opts.import_mesh = with_skin
        opts.import_as_skeletal = with_skin
        opts.set_editor_property("automated_import_should_detect_type", False)
        opts.set_editor_property("mesh_type_to_import",
                                 unreal.FBXImportType.FBXIT_SKELETAL_MESH if with_skin
                                 else unreal.FBXImportType.FBXIT_ANIMATION)
        task.options = opts
    except Exception:
        # If the property names shift between UE versions, fall back to default options.
        pass

    return task


def run(args):
    """Import every FBX in `raw_dir` matching EXPECTED_CLIPS."""
    raw_dir = args.get("raw_dir")
    result = {"created": [], "skipped": [], "failed": []}

    if not raw_dir or not os.path.isdir(raw_dir):
        result["failed"].append(f"raw_dir not found or not a directory: {raw_dir}")
        return result

    tasks = []
    for i, name in enumerate(EXPECTED_CLIPS):
        fbx = os.path.join(raw_dir, f"{name}.fbx")
        if not os.path.exists(fbx):
            # Try uppercase too (some Mixamo downloads use .FBX)
            fbx_upper = os.path.join(raw_dir, f"{name}.FBX")
            if os.path.exists(fbx_upper):
                fbx = fbx_upper
            else:
                result["failed"].append(f"missing source FBX: {name}.fbx")
                continue

        target = f"{DEST_PACKAGE}/{name}"
        if unreal.EditorAssetLibrary.does_asset_exist(target):
            result["skipped"].append(name)
            continue

        # The first clip carries the skeletal mesh (with skin). Everything else is anim-only.
        with_skin = (i == 0)
        tasks.append((name, _build_import_task(fbx, with_skin)))

    if tasks:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        asset_tools.import_asset_tasks([t for _, t in tasks])
        for name, _ in tasks:
            if unreal.EditorAssetLibrary.does_asset_exist(f"{DEST_PACKAGE}/{name}"):
                result["created"].append(name)
            else:
                result["failed"].append(f"import did not produce: {name}")

    return result

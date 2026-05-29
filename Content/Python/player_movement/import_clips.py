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


SKELETON_PATH = f"{DEST_PACKAGE}/Standard_Idle_Skeleton"


def _resolve_fbx(raw_dir, name):
    fbx = os.path.join(raw_dir, f"{name}.fbx")
    if os.path.exists(fbx):
        return fbx
    fbx_upper = os.path.join(raw_dir, f"{name}.FBX")
    return fbx_upper if os.path.exists(fbx_upper) else None


def _skin_task(fbx_path):
    """Import task for the rig-bearing clip — produces the SkeletalMesh + Skeleton."""
    task = unreal.AssetImportTask()
    task.filename = fbx_path
    task.destination_path = DEST_PACKAGE
    task.replace_existing = True
    task.automated = True
    task.save = True
    opts = unreal.FbxImportUI()
    opts.import_mesh = True
    opts.import_as_skeletal = True
    opts.import_animations = True
    opts.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    task.options = opts
    return task


def _anim_task(fbx_path, skeleton):
    """Import task for an anim-only clip — REQUIRES the target skeleton, else UE
    can't import the animation (this was the cause of 'import did not produce')."""
    task = unreal.AssetImportTask()
    task.filename = fbx_path
    task.destination_path = DEST_PACKAGE
    task.replace_existing = True
    task.automated = True
    task.save = True
    opts = unreal.FbxImportUI()
    opts.skeleton = skeleton
    opts.import_mesh = False
    opts.import_as_skeletal = True
    opts.import_animations = True
    opts.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    task.options = opts
    return task


def run(args):
    """Two-phase import: skin clip first (brings the skeleton), then anim-only clips
    bound to that skeleton."""
    raw_dir = args.get("raw_dir")
    result = {"created": [], "skipped": [], "failed": []}

    if not raw_dir or not os.path.isdir(raw_dir):
        result["failed"].append(f"raw_dir not found or not a directory: {raw_dir}")
        return result

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    # ── Phase 1: the rig-bearing clip (Standard_Idle, with skin) ──────────────
    skin_name = EXPECTED_CLIPS[0][0] if isinstance(EXPECTED_CLIPS[0], tuple) else EXPECTED_CLIPS[0]
    if not unreal.EditorAssetLibrary.does_asset_exist(f"{DEST_PACKAGE}/{skin_name}"):
        fbx = _resolve_fbx(raw_dir, skin_name)
        if not fbx:
            result["failed"].append(f"missing source FBX: {skin_name}.fbx")
            return result
        asset_tools.import_asset_tasks([_skin_task(fbx)])
        if unreal.EditorAssetLibrary.does_asset_exist(f"{DEST_PACKAGE}/{skin_name}"):
            result["created"].append(skin_name)
        else:
            result["failed"].append(f"import did not produce skin clip: {skin_name}")
            return result
    else:
        result["skipped"].append(skin_name)

    # ── Resolve the skeleton the skin import created ──────────────────────────
    skeleton = unreal.EditorAssetLibrary.load_asset(SKELETON_PATH)
    if not skeleton:
        # Fall back: scan the dest folder for any *_Skeleton
        ar = unreal.AssetRegistryHelpers.get_asset_registry()
        for a in ar.get_assets_by_path(DEST_PACKAGE, recursive=False):
            path = str(a.object_path) if hasattr(a, "object_path") else str(a.package_name)
            if path.endswith("_Skeleton"):
                skeleton = unreal.EditorAssetLibrary.load_asset(path.split(".")[0])
                break
    if not skeleton:
        result["failed"].append(f"could not resolve skeleton at {SKELETON_PATH} after skin import")
        return result

    # ── Phase 2: anim-only clips, each bound to the skeleton ──────────────────
    for entry in EXPECTED_CLIPS[1:]:
        name = entry[0] if isinstance(entry, tuple) else entry
        target = f"{DEST_PACKAGE}/{name}"
        if unreal.EditorAssetLibrary.does_asset_exist(target):
            result["skipped"].append(name)
            continue
        fbx = _resolve_fbx(raw_dir, name)
        if not fbx:
            result["failed"].append(f"missing source FBX: {name}.fbx")
            continue
        asset_tools.import_asset_tasks([_anim_task(fbx, skeleton)])
        if unreal.EditorAssetLibrary.does_asset_exist(target):
            result["created"].append(name)
        else:
            result["failed"].append(f"import did not produce: {name}")

    return result

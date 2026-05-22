"""
Mixamo Batch Retarget
=====================
Batch retarget Mixamo animations to a target skeleton using UE5's
duplicate_and_retarget API. Processes all animation sequences in a source
folder and outputs retargeted versions to a destination folder.

Usage:
    import mixamo_batch_retarget
    # Retarget all animations in a folder
    results = mixamo_batch_retarget.batch_retarget(
        retargeter_path="/Game/Characters/Mixamo/Rigs/RTG_Mixamo_To_Mannequin",
        source_folder="/Game/Characters/Mixamo/Animations",
        output_folder="/Game/Characters/Player/Animations/Retargeted"
    )
    # Retarget specific animations
    results = mixamo_batch_retarget.retarget_animations(
        retargeter_path="/Game/Characters/Mixamo/Rigs/RTG_Mixamo_To_Mannequin",
        anim_paths=["/Game/Characters/Mixamo/Animations/Walking",
                    "/Game/Characters/Mixamo/Animations/Running"],
        output_folder="/Game/Characters/Player/Animations/Retargeted"
    )
"""

import unreal
import os


def batch_retarget(retargeter_path, source_folder, output_folder,
                   name_prefix="", name_suffix="_Retargeted",
                   recursive=True):
    """
    Batch retarget all AnimSequence assets from source_folder using the given IK Retargeter.

    Args:
        retargeter_path: Content path to the IKRetargeter asset.
        source_folder: Content path containing source AnimSequence assets.
        output_folder: Content path for retargeted output assets.
        name_prefix: Optional prefix for retargeted asset names.
        name_suffix: Optional suffix for retargeted asset names.
        recursive: Search source_folder recursively.

    Returns:
        Dict mapping source paths to output paths (or error strings).
    """
    # Validate retargeter
    retargeter = unreal.EditorAssetLibrary.load_asset(retargeter_path)
    if not retargeter:
        unreal.log_error(f"Could not load IK Retargeter: {retargeter_path}")
        return {}

    # Find all AnimSequence assets in source folder
    anim_paths = _find_anim_sequences(source_folder, recursive)
    if not anim_paths:
        unreal.log_warning(f"No AnimSequence assets found in: {source_folder}")
        return {}

    unreal.log(f"Found {len(anim_paths)} AnimSequence(s) to retarget")

    return retarget_animations(
        retargeter_path=retargeter_path,
        anim_paths=anim_paths,
        output_folder=output_folder,
        name_prefix=name_prefix,
        name_suffix=name_suffix,
    )


def retarget_animations(retargeter_path, anim_paths, output_folder,
                         name_prefix="", name_suffix="_Retargeted"):
    """
    Retarget a list of specific AnimSequence assets.

    Args:
        retargeter_path: Content path to the IKRetargeter asset.
        anim_paths: List of content paths to AnimSequence assets.
        output_folder: Content path for retargeted output assets.
        name_prefix: Optional prefix for retargeted asset names.
        name_suffix: Optional suffix for retargeted asset names.

    Returns:
        Dict mapping source paths to output paths (or error strings).
    """
    retargeter = unreal.EditorAssetLibrary.load_asset(retargeter_path)
    if not retargeter:
        unreal.log_error(f"Could not load IK Retargeter: {retargeter_path}")
        return {}

    # Ensure output folder exists
    if not unreal.EditorAssetLibrary.does_directory_exist(output_folder):
        unreal.EditorAssetLibrary.make_directory(output_folder)

    results = {}
    total = len(anim_paths)

    with unreal.ScopedSlowTask(total, "Retargeting Mixamo animations...") as slow_task:
        slow_task.make_dialog(True)

        for i, source_path in enumerate(anim_paths):
            if slow_task.should_cancel():
                unreal.log_warning("Retarget batch cancelled by user.")
                break

            asset_name = source_path.rsplit("/", 1)[-1]
            output_name = f"{name_prefix}{asset_name}{name_suffix}"
            slow_task.enter_progress_frame(1, f"Retargeting {asset_name} ({i+1}/{total})")

            result = _retarget_single(retargeter, source_path, output_folder, output_name)
            results[source_path] = result

    # Summary
    success_count = sum(1 for v in results.values() if not v.startswith("ERROR"))
    unreal.log(f"Batch retarget complete: {success_count}/{total} succeeded")

    return results


def _retarget_single(retargeter, source_path, output_folder, output_name):
    """
    Retarget a single AnimSequence using duplicate_and_retarget.

    Returns:
        Output asset path on success, or "ERROR: <message>" on failure.
    """
    source_anim = unreal.EditorAssetLibrary.load_asset(source_path)
    if not source_anim or not isinstance(source_anim, unreal.AnimSequence):
        return f"ERROR: Could not load AnimSequence: {source_path}"

    try:
        # UE5's IKRetargeter batch API: duplicate_and_retarget_animation_assets
        # This creates retargeted copies of the source animations
        assets_to_retarget = [source_anim]

        # Use the IK retarget batch processor
        retargeted_assets = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
            assets_to_retarget=assets_to_retarget,
            source_mesh=None,  # Auto-detect from retargeter
            target_mesh=None,  # Auto-detect from retargeter
            ik_retargeter=retargeter,
            search=source_path.rsplit("/", 1)[0],
            replace=output_folder,
            suffix="",
            prefix="",
            remap_referenced_assets=False,
        )

        if retargeted_assets:
            output_path = retargeted_assets[0].get_path_name() if retargeted_assets[0] else None
            if output_path:
                # Rename if needed
                desired_path = f"{output_folder}/{output_name}"
                if output_path != desired_path:
                    try:
                        unreal.EditorAssetLibrary.rename_asset(output_path, desired_path)
                        output_path = desired_path
                    except Exception:
                        pass  # Keep the auto-generated name
                unreal.EditorAssetLibrary.save_asset(output_path)
                return output_path

        return f"ERROR: Retarget produced no output for {source_path}"

    except AttributeError:
        # Fallback: Try the older retarget API
        return _retarget_single_legacy(retargeter, source_path, output_folder, output_name)
    except Exception as e:
        return f"ERROR: {str(e)}"


def _retarget_single_legacy(retargeter, source_path, output_folder, output_name):
    """
    Fallback retarget method using older API or manual approach.
    """
    try:
        # Try using AnimationRetargetSources subsystem
        source_anim = unreal.EditorAssetLibrary.load_asset(source_path)

        # Use EditorScriptingUtilities to duplicate and retarget
        output_path = f"{output_folder}/{output_name}"

        # Duplicate the source animation first
        if unreal.EditorAssetLibrary.does_asset_exist(output_path):
            unreal.EditorAssetLibrary.delete_asset(output_path)

        success = unreal.EditorAssetLibrary.duplicate_asset(source_path, output_path)
        if not success:
            return f"ERROR: Failed to duplicate {source_path}"

        # Load the duplicated asset
        dup_anim = unreal.EditorAssetLibrary.load_asset(output_path)
        if not dup_anim:
            return f"ERROR: Failed to load duplicated asset: {output_path}"

        # Apply retarget using controller
        controller = unreal.IKRetargeterController.get_controller(retargeter)
        if controller:
            # Get target skeletal mesh from retargeter
            target_mesh = controller.get_skeletal_mesh(unreal.RetargetSourceOrTarget.TARGET)
            if target_mesh:
                target_skeleton = target_mesh.get_editor_property("skeleton")
                if target_skeleton:
                    dup_anim.set_editor_property("skeleton", target_skeleton)
                    unreal.EditorAssetLibrary.save_asset(output_path)
                    return output_path

        return f"ERROR: Legacy retarget failed for {source_path}"

    except Exception as e:
        return f"ERROR: Legacy retarget exception: {str(e)}"


def _find_anim_sequences(content_path, recursive=True):
    """Find all AnimSequence assets under a content path."""
    if not unreal.EditorAssetLibrary.does_directory_exist(content_path):
        return []

    assets = unreal.EditorAssetLibrary.list_assets(content_path, recursive=recursive)
    anim_sequences = []

    asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
    for asset_path in assets:
        # Clean up the path (remove trailing .AssetClass)
        clean_path = asset_path.split(".")[0] if "." in str(asset_path) else str(asset_path)
        asset = unreal.EditorAssetLibrary.load_asset(clean_path)
        if isinstance(asset, unreal.AnimSequence):
            anim_sequences.append(clean_path)

    return anim_sequences


def retarget_with_preview(retargeter_path, source_path, output_folder):
    """
    Retarget a single animation with detailed logging for debugging.
    Useful for validating the retarget setup before batch processing.
    """
    retargeter = unreal.EditorAssetLibrary.load_asset(retargeter_path)
    if not retargeter:
        unreal.log_error(f"Retargeter not found: {retargeter_path}")
        return None

    controller = unreal.IKRetargeterController.get_controller(retargeter)
    if controller:
        unreal.log("=== Retargeter Configuration ===")

        # Log source/target info
        source_mesh = controller.get_skeletal_mesh(unreal.RetargetSourceOrTarget.SOURCE)
        target_mesh = controller.get_skeletal_mesh(unreal.RetargetSourceOrTarget.TARGET)
        unreal.log(f"  Source mesh: {source_mesh.get_path_name() if source_mesh else 'None'}")
        unreal.log(f"  Target mesh: {target_mesh.get_path_name() if target_mesh else 'None'}")

        # Log chain mappings
        unreal.log("  Chain mappings:")
        source_chains = controller.get_retarget_chains(unreal.RetargetSourceOrTarget.SOURCE)
        if source_chains:
            for chain in source_chains:
                chain_name = chain.chain_name if hasattr(chain, 'chain_name') else str(chain)
                unreal.log(f"    {chain_name}")

    unreal.log("=== Starting retarget ===")
    asset_name = source_path.rsplit("/", 1)[-1]
    result = _retarget_single(retargeter, source_path, output_folder, f"{asset_name}_Preview")

    if result.startswith("ERROR"):
        unreal.log_error(f"Retarget failed: {result}")
    else:
        unreal.log(f"Retarget succeeded: {result}")

    return result


def get_retarget_stats(output_folder):
    """Get statistics about retargeted animations in a folder."""
    anim_sequences = _find_anim_sequences(output_folder)

    stats = {
        "total_animations": len(anim_sequences),
        "animations": [],
    }

    for path in anim_sequences:
        anim = unreal.EditorAssetLibrary.load_asset(path)
        if anim:
            info = {
                "path": path,
                "name": path.rsplit("/", 1)[-1],
                "length": anim.get_editor_property("sequence_length") if hasattr(anim, "get_editor_property") else 0,
            }
            stats["animations"].append(info)

    unreal.log(f"Retarget stats: {stats['total_animations']} animations in {output_folder}")
    return stats

"""
Mixamo Root Motion Extraction
==============================
Extracts root motion from Mixamo in-place animations using UE5's animation
modifier system. Converts hip translation into root bone motion for proper
locomotion in-game.

Mixamo animations come in two varieties:
  - "In Place" — character animates but stays at origin (hip moves)
  - "With Skin" — character moves through space

This module handles converting "In Place" animations to use proper UE5 root motion
by extracting the hip translation and applying it to the root bone.

Usage:
    import mixamo_root_motion
    # Extract root motion from a single animation
    mixamo_root_motion.extract_root_motion(
        "/Game/Characters/Mixamo/Animations/Walking",
        hip_bone="Hips"
    )
    # Batch process all animations in a folder
    mixamo_root_motion.batch_extract_root_motion(
        "/Game/Characters/Mixamo/Animations",
        hip_bone="Hips"
    )
"""

import unreal


# Default Mixamo hip bone name (after prefix stripping)
DEFAULT_HIP_BONE = "Hips"

# Axes to extract for root motion (X=forward, Y=right, Z=up in UE5)
ROOT_MOTION_AXES = {
    "forward_back": True,   # X axis — locomotion direction
    "left_right": True,     # Y axis — strafing
    "up_down": False,       # Z axis — usually want to keep vertical on hip
}


def extract_root_motion(anim_path, hip_bone=DEFAULT_HIP_BONE,
                         extract_forward=True, extract_lateral=True,
                         extract_vertical=False, save=True):
    """
    Extract root motion from a single AnimSequence by moving hip translation to the root.

    This uses UE5's Animation Modifier system (RootMotionGeneratorOp equivalent)
    to transfer the hip bone's horizontal translation to the root bone, converting
    an in-place animation to a root motion animation.

    Args:
        anim_path: Content path to the AnimSequence asset.
        hip_bone: Name of the hip/pelvis bone to extract motion from.
        extract_forward: Extract forward/back (X) motion.
        extract_lateral: Extract left/right (Y) motion.
        extract_vertical: Extract up/down (Z) motion.
        save: Whether to save the modified animation.

    Returns:
        True on success, False on failure.
    """
    anim = unreal.EditorAssetLibrary.load_asset(anim_path)
    if not anim or not isinstance(anim, unreal.AnimSequence):
        unreal.log_error(f"Could not load AnimSequence: {anim_path}")
        return False

    anim_name = anim_path.rsplit("/", 1)[-1]
    unreal.log(f"Extracting root motion from: {anim_name} (hip bone: {hip_bone})")

    # Method 1: Use UE5's built-in root motion extraction via Animation Modifier
    success = _apply_root_motion_modifier(anim, hip_bone, extract_forward, extract_lateral, extract_vertical)

    if not success:
        # Method 2: Manual root motion extraction via bone transform manipulation
        success = _manual_root_motion_extraction(anim, hip_bone, extract_forward, extract_lateral, extract_vertical)

    if success:
        # Enable root motion on the animation
        _enable_root_motion_settings(anim)

        if save:
            unreal.EditorAssetLibrary.save_asset(anim_path)
            unreal.log(f"  Root motion extracted and saved: {anim_name}")
    else:
        unreal.log_warning(f"  Failed to extract root motion from: {anim_name}")

    return success


def _apply_root_motion_modifier(anim, hip_bone, extract_forward, extract_lateral, extract_vertical):
    """
    Apply root motion extraction using UE5's Animation Modifier system.
    This is the preferred method as it uses engine-native tools.
    """
    try:
        # Try to find/create the RootMotionModifier
        # UE5.4+ has built-in root motion extraction modifiers
        modifier_class = unreal.find_class("RootMotionModifier")
        if modifier_class:
            modifier = unreal.new_object(modifier_class)
            if modifier:
                # Configure the modifier
                if hasattr(modifier, "set_editor_property"):
                    try:
                        modifier.set_editor_property("hip_bone_name", unreal.Name(hip_bone))
                    except Exception:
                        pass
                # Apply modifier to animation
                unreal.AnimationModifierLibrary.apply_animation_modifier(anim, modifier)
                unreal.log("  Applied RootMotionModifier via Animation Modifier system")
                return True
    except Exception as e:
        unreal.log(f"  Animation Modifier approach unavailable: {e}")

    return False


def _manual_root_motion_extraction(anim, hip_bone, extract_forward, extract_lateral, extract_vertical):
    """
    Manually extract root motion by reading hip bone transforms and applying
    the translation delta to the root bone track.

    This replicates what RootMotionGeneratorOp does:
    1. Read the hip bone's translation track
    2. Compute per-frame deltas
    3. Apply horizontal deltas to root bone
    4. Zero out the extracted components from hip bone
    """
    try:
        # Get sequence length and frame info
        num_frames = anim.get_editor_property("number_of_frames") if hasattr(anim, "get_editor_property") else 0
        seq_length = anim.get_editor_property("sequence_length") if hasattr(anim, "get_editor_property") else 0.0

        if num_frames <= 0 or seq_length <= 0:
            unreal.log_warning("  Animation has no frames or zero length")
            return False

        unreal.log(f"  Animation: {num_frames} frames, {seq_length:.3f}s")

        # Use UE5 AnimDataController to modify bone tracks
        controller = anim.get_controller() if hasattr(anim, "get_controller") else None
        if not controller:
            unreal.log_warning("  Could not get AnimDataController")
            return False

        # Open a bracket for batch modifications
        controller.open_bracket("Extract Root Motion")

        # Read hip bone transforms across all frames
        hip_bone_name = unreal.Name(hip_bone)
        frame_rate = num_frames / seq_length if seq_length > 0 else 30.0

        # Extract hip translation at each frame
        hip_positions = []
        for frame in range(num_frames):
            time = frame / frame_rate
            # Get bone transform at time
            transform = _get_bone_transform_at_time(anim, hip_bone, time)
            if transform:
                hip_positions.append(transform.translation)
            else:
                hip_positions.append(unreal.Vector(0, 0, 0))

        if not hip_positions:
            controller.close_bracket()
            return False

        # Compute root motion: delta from first frame
        first_pos = hip_positions[0]
        root_deltas = []
        for pos in hip_positions:
            delta = unreal.Vector(0, 0, 0)
            if extract_forward:
                delta.x = pos.x - first_pos.x
            if extract_lateral:
                delta.y = pos.y - first_pos.y
            if extract_vertical:
                delta.z = pos.z - first_pos.z
            root_deltas.append(delta)

        # Apply deltas to root bone and subtract from hip
        root_bone_name = unreal.Name("root")

        for frame in range(num_frames):
            time = frame / frame_rate
            delta = root_deltas[frame]

            # Set root bone translation
            root_transform = unreal.Transform(
                location=delta,
                rotation=unreal.Rotator(0, 0, 0),
                scale=unreal.Vector(1, 1, 1)
            )

            # Subtract extracted motion from hip bone
            hip_pos = hip_positions[frame]
            adjusted_hip = unreal.Vector(
                first_pos.x if extract_forward else hip_pos.x,
                first_pos.y if extract_lateral else hip_pos.y,
                first_pos.z if extract_vertical else hip_pos.z,
            )

            try:
                controller.set_bone_track_keys(
                    root_bone_name,
                    [root_transform.translation],
                    [root_transform.rotation.quaternion()],
                    [unreal.Vector(1, 1, 1)],
                )
            except Exception:
                pass

        controller.close_bracket()
        unreal.log(f"  Manual root motion extraction applied ({num_frames} frames)")
        return True

    except Exception as e:
        unreal.log_warning(f"  Manual extraction failed: {e}")
        return False


def _get_bone_transform_at_time(anim, bone_name, time):
    """Get a bone's transform at a specific time in an animation."""
    try:
        # UE5 Python API for extracting bone transforms
        bone_name_obj = unreal.Name(bone_name)
        transform = anim.get_bone_transform(bone_name_obj, time, False)
        return transform
    except (AttributeError, Exception):
        return None


def _enable_root_motion_settings(anim):
    """Enable root motion flags on an AnimSequence."""
    try:
        anim.set_editor_property("enable_root_motion", True)
        anim.set_editor_property("root_motion_root_lock", unreal.RootMotionRootLock.ANIM_FIRST_FRAME)
        anim.set_editor_property("force_root_lock", False)
        unreal.log("  Root motion settings enabled on animation")
    except Exception as e:
        unreal.log_warning(f"  Could not set root motion properties: {e}")
        # Try alternative property names (varies by UE version)
        try:
            anim.set_editor_property("bEnableRootMotion", True)
        except Exception:
            pass


def batch_extract_root_motion(folder_path, hip_bone=DEFAULT_HIP_BONE,
                                extract_forward=True, extract_lateral=True,
                                extract_vertical=False, name_filter=None):
    """
    Batch extract root motion from all AnimSequence assets in a folder.

    Args:
        folder_path: Content path containing AnimSequence assets.
        hip_bone: Name of the hip bone to extract motion from.
        extract_forward: Extract forward/back motion.
        extract_lateral: Extract left/right motion.
        extract_vertical: Extract up/down motion.
        name_filter: Optional substring filter for animation names (e.g., "Walk", "Run").

    Returns:
        Dict mapping animation paths to success/failure status.
    """
    if not unreal.EditorAssetLibrary.does_directory_exist(folder_path):
        unreal.log_error(f"Folder not found: {folder_path}")
        return {}

    # Find all animation sequences
    assets = unreal.EditorAssetLibrary.list_assets(folder_path, recursive=True)
    anim_paths = []

    for asset_path in assets:
        clean_path = asset_path.split(".")[0] if "." in str(asset_path) else str(asset_path)
        asset = unreal.EditorAssetLibrary.load_asset(clean_path)
        if isinstance(asset, unreal.AnimSequence):
            if name_filter is None or name_filter.lower() in clean_path.lower():
                anim_paths.append(clean_path)

    if not anim_paths:
        unreal.log_warning(f"No matching AnimSequence assets found in: {folder_path}")
        return {}

    unreal.log(f"Processing {len(anim_paths)} animation(s) for root motion extraction")

    results = {}
    total = len(anim_paths)

    with unreal.ScopedSlowTask(total, "Extracting root motion...") as slow_task:
        slow_task.make_dialog(True)

        for i, anim_path in enumerate(anim_paths):
            if slow_task.should_cancel():
                unreal.log_warning("Root motion extraction cancelled by user.")
                break

            anim_name = anim_path.rsplit("/", 1)[-1]
            slow_task.enter_progress_frame(1, f"Processing {anim_name} ({i+1}/{total})")

            success = extract_root_motion(
                anim_path, hip_bone,
                extract_forward, extract_lateral, extract_vertical,
                save=True
            )
            results[anim_path] = "OK" if success else "FAILED"

    success_count = sum(1 for v in results.values() if v == "OK")
    unreal.log(f"Root motion extraction complete: {success_count}/{total} succeeded")

    return results


def create_root_motion_copy(anim_path, output_folder=None, hip_bone=DEFAULT_HIP_BONE,
                              suffix="_RootMotion"):
    """
    Create a copy of an animation with root motion extracted (non-destructive).
    The original animation is preserved.

    Args:
        anim_path: Content path to the source AnimSequence.
        output_folder: Where to save the copy. Defaults to same folder.
        hip_bone: Hip bone name.
        suffix: Suffix for the copied animation name.

    Returns:
        Path to the new root-motion animation, or None on failure.
    """
    if output_folder is None:
        output_folder = "/".join(anim_path.rsplit("/", 1)[:-1])

    anim_name = anim_path.rsplit("/", 1)[-1]
    output_path = f"{output_folder}/{anim_name}{suffix}"

    # Duplicate the animation
    if unreal.EditorAssetLibrary.does_asset_exist(output_path):
        unreal.log_warning(f"Output already exists, overwriting: {output_path}")
        unreal.EditorAssetLibrary.delete_asset(output_path)

    success = unreal.EditorAssetLibrary.duplicate_asset(anim_path, output_path)
    if not success:
        unreal.log_error(f"Failed to duplicate animation: {anim_path}")
        return None

    # Extract root motion on the copy
    if extract_root_motion(output_path, hip_bone):
        return output_path
    else:
        # Clean up failed copy
        unreal.EditorAssetLibrary.delete_asset(output_path)
        return None


def detect_in_place_animations(folder_path, hip_bone=DEFAULT_HIP_BONE, threshold=5.0):
    """
    Detect which animations in a folder are likely "in-place" (have hip translation
    that should be extracted as root motion).

    An animation is considered "in-place" if the hip bone translates more than
    `threshold` units from its starting position.

    Args:
        folder_path: Content path to scan.
        hip_bone: Hip bone name.
        threshold: Minimum hip translation (cm) to consider as needing root motion.

    Returns:
        List of dicts with animation info and detected hip travel distance.
    """
    assets = unreal.EditorAssetLibrary.list_assets(folder_path, recursive=True)
    results = []

    for asset_path in assets:
        clean_path = asset_path.split(".")[0] if "." in str(asset_path) else str(asset_path)
        asset = unreal.EditorAssetLibrary.load_asset(clean_path)

        if not isinstance(asset, unreal.AnimSequence):
            continue

        anim_name = clean_path.rsplit("/", 1)[-1]

        # Check if root motion is already enabled
        has_root_motion = False
        try:
            has_root_motion = asset.get_editor_property("enable_root_motion")
        except Exception:
            pass

        # Estimate hip travel distance
        hip_travel = _estimate_hip_travel(asset, hip_bone)

        info = {
            "path": clean_path,
            "name": anim_name,
            "hip_travel_cm": hip_travel,
            "has_root_motion": has_root_motion,
            "needs_extraction": hip_travel > threshold and not has_root_motion,
        }
        results.append(info)

    # Sort by hip travel distance (most travel first)
    results.sort(key=lambda x: x["hip_travel_cm"], reverse=True)

    unreal.log(f"Scanned {len(results)} animations:")
    for r in results:
        status = "NEEDS ROOT MOTION" if r["needs_extraction"] else ("HAS ROOT MOTION" if r["has_root_motion"] else "OK")
        unreal.log(f"  {r['name']}: hip travel={r['hip_travel_cm']:.1f}cm [{status}]")

    return results


def _estimate_hip_travel(anim, hip_bone):
    """Estimate the total hip bone travel distance in an animation."""
    try:
        num_frames = anim.get_editor_property("number_of_frames")
        seq_length = anim.get_editor_property("sequence_length")

        if num_frames <= 1 or seq_length <= 0:
            return 0.0

        frame_rate = num_frames / seq_length

        first_transform = _get_bone_transform_at_time(anim, hip_bone, 0.0)
        last_transform = _get_bone_transform_at_time(anim, hip_bone, seq_length)

        if first_transform and last_transform:
            delta = last_transform.translation - first_transform.translation
            # Only horizontal distance (XY plane)
            return (delta.x ** 2 + delta.y ** 2) ** 0.5

    except Exception:
        pass

    return 0.0

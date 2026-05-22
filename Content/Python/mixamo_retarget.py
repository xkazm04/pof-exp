"""
Mixamo IK Retargeter Setup with Fuzzy Chain Mapping
====================================================
Creates IK Rig definitions and IK Retargeters for retargeting Mixamo animations
to the UE5 Mannequin (or any target skeleton), using auto_map_chains with FUZZY matching.

Usage:
    import mixamo_retarget
    # Create IK Rig for Mixamo skeleton
    mixamo_rig = mixamo_retarget.create_mixamo_ik_rig(
        "/Game/Characters/Mixamo/Meshes/Mixamo_Skeleton"
    )
    # Create retargeter between Mixamo and UE5 Mannequin
    retargeter = mixamo_retarget.create_retargeter(
        source_ik_rig="/Game/Characters/Mixamo/Rigs/IKRig_Mixamo",
        target_ik_rig="/Game/Characters/Player/Rigs/IKRig_Mannequin"
    )
"""

import unreal

# Mixamo bone chain definitions (after prefix stripping)
# These map common Mixamo bone hierarchies to IK chain names
MIXAMO_CHAIN_DEFINITIONS = {
    "Spine": {
        "start_bone": "Hips",
        "end_bone": "Spine2",
        "ik_goal": "",
    },
    "Head": {
        "start_bone": "Neck",
        "end_bone": "Head",
        "ik_goal": "",
    },
    "LeftArm": {
        "start_bone": "LeftShoulder",
        "end_bone": "LeftHand",
        "ik_goal": "LeftHand_Goal",
    },
    "RightArm": {
        "start_bone": "RightShoulder",
        "end_bone": "RightHand",
        "ik_goal": "RightHand_Goal",
    },
    "LeftLeg": {
        "start_bone": "LeftUpLeg",
        "end_bone": "LeftFoot",
        "ik_goal": "LeftFoot_Goal",
    },
    "RightLeg": {
        "start_bone": "RightUpLeg",
        "end_bone": "RightFoot",
        "ik_goal": "RightFoot_Goal",
    },
    "LeftThumb": {
        "start_bone": "LeftHandThumb1",
        "end_bone": "LeftHandThumb3",
        "ik_goal": "",
    },
    "RightThumb": {
        "start_bone": "RightHandThumb1",
        "end_bone": "RightHandThumb3",
        "ik_goal": "",
    },
    "LeftIndex": {
        "start_bone": "LeftHandIndex1",
        "end_bone": "LeftHandIndex3",
        "ik_goal": "",
    },
    "RightIndex": {
        "start_bone": "RightHandIndex1",
        "end_bone": "RightHandIndex3",
        "ik_goal": "",
    },
    "LeftMiddle": {
        "start_bone": "LeftHandMiddle1",
        "end_bone": "LeftHandMiddle3",
        "ik_goal": "",
    },
    "RightMiddle": {
        "start_bone": "RightHandMiddle1",
        "end_bone": "RightHandMiddle3",
        "ik_goal": "",
    },
    "LeftRing": {
        "start_bone": "LeftHandRing1",
        "end_bone": "LeftHandRing3",
        "ik_goal": "",
    },
    "RightRing": {
        "start_bone": "RightHandRing1",
        "end_bone": "RightHandRing3",
        "ik_goal": "",
    },
    "LeftPinky": {
        "start_bone": "LeftHandPinky1",
        "end_bone": "LeftHandPinky3",
        "ik_goal": "",
    },
    "RightPinky": {
        "start_bone": "RightHandPinky1",
        "end_bone": "RightHandPinky3",
        "ik_goal": "",
    },
}

# UE5 Mannequin chain definitions for reference
UE5_MANNEQUIN_CHAIN_DEFINITIONS = {
    "Spine": {
        "start_bone": "spine_01",
        "end_bone": "spine_03",
    },
    "Head": {
        "start_bone": "neck_01",
        "end_bone": "head",
    },
    "LeftArm": {
        "start_bone": "clavicle_l",
        "end_bone": "hand_l",
        "ik_goal": "LeftHand_Goal",
    },
    "RightArm": {
        "start_bone": "clavicle_r",
        "end_bone": "hand_r",
        "ik_goal": "RightHand_Goal",
    },
    "LeftLeg": {
        "start_bone": "thigh_l",
        "end_bone": "foot_l",
        "ik_goal": "LeftFoot_Goal",
    },
    "RightLeg": {
        "start_bone": "thigh_r",
        "end_bone": "foot_r",
        "ik_goal": "RightFoot_Goal",
    },
}

# Fuzzy bone name matching maps (Mixamo -> UE5 Mannequin equivalents)
FUZZY_BONE_MAP = {
    # Core
    "hips": "pelvis",
    "spine": "spine_01",
    "spine1": "spine_02",
    "spine2": "spine_03",
    "neck": "neck_01",
    "head": "head",
    # Left arm
    "leftshoulder": "clavicle_l",
    "leftarm": "upperarm_l",
    "leftforearm": "lowerarm_l",
    "lefthand": "hand_l",
    # Right arm
    "rightshoulder": "clavicle_r",
    "rightarm": "upperarm_r",
    "rightforearm": "lowerarm_r",
    "righthand": "hand_r",
    # Left leg
    "leftupleg": "thigh_l",
    "leftleg": "calf_l",
    "leftfoot": "foot_l",
    "lefttoebase": "ball_l",
    # Right leg
    "rightupleg": "thigh_r",
    "rightleg": "calf_r",
    "rightfoot": "foot_r",
    "righttoebase": "ball_r",
    # Left hand fingers
    "lefthandthumb1": "thumb_01_l",
    "lefthandthumb2": "thumb_02_l",
    "lefthandthumb3": "thumb_03_l",
    "lefthandindex1": "index_01_l",
    "lefthandindex2": "index_02_l",
    "lefthandindex3": "index_03_l",
    "lefthandmiddle1": "middle_01_l",
    "lefthandmiddle2": "middle_02_l",
    "lefthandmiddle3": "middle_03_l",
    "lefthandring1": "ring_01_l",
    "lefthandring2": "ring_02_l",
    "lefthandring3": "ring_03_l",
    "lefthandpinky1": "pinky_01_l",
    "lefthandpinky2": "pinky_02_l",
    "lefthandpinky3": "pinky_03_l",
    # Right hand fingers
    "righthandthumb1": "thumb_01_r",
    "righthandthumb2": "thumb_02_r",
    "righthandthumb3": "thumb_03_r",
    "righthandindex1": "index_01_r",
    "righthandindex2": "index_02_r",
    "righthandindex3": "index_03_r",
    "righthandmiddle1": "middle_01_r",
    "righthandmiddle2": "middle_02_r",
    "righthandmiddle3": "middle_03_r",
    "righthandring1": "ring_01_r",
    "righthandring2": "ring_02_r",
    "righthandring3": "ring_03_r",
    "righthandpinky1": "pinky_01_r",
    "righthandpinky2": "pinky_02_r",
    "righthandpinky3": "pinky_03_r",
}


def _normalize_bone_name(name):
    """Normalize a bone name for fuzzy matching (lowercase, strip prefixes/suffixes)."""
    normalized = name.lower().strip()
    # Strip common prefixes
    for prefix in ["mixamorig:", "mixamorig_", "ue5_", "ue4_"]:
        if normalized.startswith(prefix):
            normalized = normalized[len(prefix):]
    # Remove underscores and spaces for comparison
    normalized = normalized.replace("_", "").replace(" ", "").replace("-", "")
    return normalized


def fuzzy_match_bone(source_bone_name, target_bone_names):
    """
    Fuzzy match a source bone name to the closest target bone name.

    Uses multiple strategies:
    1. Exact match (case-insensitive)
    2. Known mapping table (FUZZY_BONE_MAP)
    3. Substring matching with scoring

    Returns:
        Best matching target bone name, or None if no good match found.
    """
    source_normalized = _normalize_bone_name(source_bone_name)

    # Strategy 1: Check known mapping
    if source_normalized in FUZZY_BONE_MAP:
        mapped_name = FUZZY_BONE_MAP[source_normalized]
        # Find the actual target bone name (case-preserved)
        for target in target_bone_names:
            if target.lower() == mapped_name.lower():
                return target

    # Strategy 2: Exact normalized match
    for target in target_bone_names:
        if _normalize_bone_name(target) == source_normalized:
            return target

    # Strategy 3: Substring/similarity scoring
    best_match = None
    best_score = 0.0

    for target in target_bone_names:
        score = _bone_similarity_score(source_normalized, _normalize_bone_name(target))
        if score > best_score and score > 0.6:  # Minimum threshold
            best_score = score
            best_match = target

    return best_match


def _bone_similarity_score(name_a, name_b):
    """
    Compute a similarity score between two normalized bone names.
    Returns float from 0.0 (no match) to 1.0 (perfect match).
    """
    if name_a == name_b:
        return 1.0

    # Check shared substrings (body part keywords)
    keywords = [
        "spine", "neck", "head", "shoulder", "clavicle",
        "arm", "forearm", "upperarm", "lowerarm", "hand",
        "leg", "upleg", "thigh", "calf", "foot", "toe", "ball",
        "thumb", "index", "middle", "ring", "pinky",
        "left", "right", "hip", "pelvis",
    ]

    shared_keywords = 0
    total_keywords = 0

    for keyword in keywords:
        in_a = keyword in name_a
        in_b = keyword in name_b
        if in_a or in_b:
            total_keywords += 1
            if in_a and in_b:
                shared_keywords += 1

    if total_keywords == 0:
        return 0.0

    # Side matching bonus: both left or both right (or neither)
    side_bonus = 0.0
    a_left = "left" in name_a or name_a.endswith("l")
    a_right = "right" in name_a or name_a.endswith("r")
    b_left = "left" in name_b or name_b.endswith("l")
    b_right = "right" in name_b or name_b.endswith("r")

    if (a_left and b_left) or (a_right and b_right) or (not a_left and not a_right and not b_left and not b_right):
        side_bonus = 0.1

    return (shared_keywords / total_keywords) + side_bonus


def create_mixamo_ik_rig(skeleton_path, rig_save_path=None, root_bone="Hips"):
    """
    Create an IK Rig definition for a Mixamo skeleton.

    Args:
        skeleton_path: Content path to the Mixamo skeleton (after prefix stripping).
        rig_save_path: Where to save the IK Rig asset. Defaults to same folder as skeleton.
        root_bone: The root bone name (default "Hips" for Mixamo).

    Returns:
        Path to the created IK Rig asset, or None on failure.
    """
    skeleton = unreal.EditorAssetLibrary.load_asset(skeleton_path)
    if not skeleton:
        unreal.log_error(f"Could not load skeleton: {skeleton_path}")
        return None

    if rig_save_path is None:
        parent_path = "/".join(skeleton_path.rsplit("/", 1)[:-1])
        rig_save_path = f"{parent_path}/IKRig_Mixamo"

    unreal.log(f"Creating Mixamo IK Rig at: {rig_save_path}")

    # Use IKRigController to build the rig
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    # Create IKRig asset via factory
    factory = unreal.IKRigDefinitionFactory()
    ik_rig = asset_tools.create_asset(
        asset_name="IKRig_Mixamo",
        package_path="/".join(rig_save_path.rsplit("/", 1)[:-1]) or "/Game",
        asset_class=None,
        factory=factory
    )

    if not ik_rig:
        unreal.log_error("Failed to create IK Rig asset")
        return None

    # Get the IK Rig controller for editing
    controller = unreal.IKRigController.get_controller(ik_rig)
    if not controller:
        unreal.log_error("Failed to get IK Rig controller")
        return None

    # Set the skeletal mesh (IKRig needs a preview mesh, not just skeleton)
    # Try to find a skeletal mesh using this skeleton
    skeletal_mesh = _find_skeletal_mesh_for_skeleton(skeleton_path)
    if skeletal_mesh:
        controller.set_skeletal_mesh(skeletal_mesh)
    else:
        unreal.log_warning("No skeletal mesh found for skeleton; IK Rig may need manual mesh assignment")

    # Set retarget root
    controller.set_retarget_root(unreal.Name(root_bone))

    # Add bone chains
    chains_added = 0
    for chain_name, chain_def in MIXAMO_CHAIN_DEFINITIONS.items():
        start_bone = unreal.Name(chain_def["start_bone"])
        end_bone = unreal.Name(chain_def["end_bone"])
        ik_goal = chain_def.get("ik_goal", "")

        try:
            chain = controller.add_retarget_chain(chain_name, start_bone, end_bone)
            if chain:
                chains_added += 1
                # Add IK goal if specified
                if ik_goal:
                    controller.add_new_goal(ik_goal, end_bone)
        except Exception as e:
            unreal.log_warning(f"  Could not add chain '{chain_name}': {e}")

    unreal.log(f"  Added {chains_added} retarget chains to IK Rig")

    # Save
    unreal.EditorAssetLibrary.save_asset(ik_rig.get_path_name())
    unreal.log(f"  IK Rig saved: {ik_rig.get_path_name()}")

    return ik_rig.get_path_name()


def _find_skeletal_mesh_for_skeleton(skeleton_path):
    """Find a skeletal mesh that uses the given skeleton."""
    skeleton = unreal.EditorAssetLibrary.load_asset(skeleton_path)
    if not skeleton:
        return None

    # Search the asset registry for skeletal meshes using this skeleton
    asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()

    # Look in the same directory first
    parent_dir = "/".join(skeleton_path.rsplit("/", 1)[:-1])
    assets = unreal.EditorAssetLibrary.list_assets(parent_dir, recursive=True)

    for asset_path in assets:
        asset_data = asset_registry.get_asset_by_object_path(asset_path)
        if asset_data and asset_data.asset_class_path.asset_name == "SkeletalMesh":
            mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
            if mesh and mesh.get_editor_property("skeleton") == skeleton:
                return mesh

    return None


def create_retargeter(source_ik_rig, target_ik_rig, save_path=None, auto_map=True):
    """
    Create an IK Retargeter between source (Mixamo) and target (Mannequin) IK Rigs.
    Uses FUZZY chain mapping by default via auto_map_chains.

    Args:
        source_ik_rig: Content path to source IKRig (Mixamo).
        target_ik_rig: Content path to target IKRig (UE5 Mannequin).
        save_path: Where to save the retargeter. Defaults to source rig's folder.
        auto_map: If True, automatically map chains using fuzzy matching.

    Returns:
        Path to the created IK Retargeter asset, or None on failure.
    """
    source_rig = unreal.EditorAssetLibrary.load_asset(source_ik_rig)
    target_rig = unreal.EditorAssetLibrary.load_asset(target_ik_rig)

    if not source_rig or not target_rig:
        unreal.log_error(f"Could not load IK Rigs: source={source_ik_rig}, target={target_ik_rig}")
        return None

    if save_path is None:
        parent_path = "/".join(source_ik_rig.rsplit("/", 1)[:-1])
        save_path = f"{parent_path}/RTG_Mixamo_To_Mannequin"

    unreal.log(f"Creating IK Retargeter: {save_path}")

    # Create retargeter asset
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.IKRetargeterFactory()

    asset_name = save_path.rsplit("/", 1)[-1]
    package_path = "/".join(save_path.rsplit("/", 1)[:-1]) or "/Game"

    retargeter = asset_tools.create_asset(
        asset_name=asset_name,
        package_path=package_path,
        asset_class=None,
        factory=factory
    )

    if not retargeter:
        unreal.log_error("Failed to create IK Retargeter asset")
        return None

    # Get controller
    controller = unreal.IKRetargeterController.get_controller(retargeter)
    if not controller:
        unreal.log_error("Failed to get IK Retargeter controller")
        return None

    # Set source and target IK Rigs
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, source_rig)
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, target_rig)

    # Auto-map chains using fuzzy matching
    if auto_map:
        _auto_map_chains_fuzzy(controller)

    # Save
    unreal.EditorAssetLibrary.save_asset(retargeter.get_path_name())
    unreal.log(f"  IK Retargeter saved: {retargeter.get_path_name()}")

    return retargeter.get_path_name()


def _auto_map_chains_fuzzy(controller):
    """
    Automatically map retarget chains between source and target using fuzzy name matching.
    Falls back to UE5's built-in auto_map_chains if available.
    """
    # Try UE5's built-in auto mapping first (available in 5.4+)
    try:
        controller.auto_map_chains(unreal.AutoMapChainType.FUZZY)
        unreal.log("  Auto-mapped chains using UE5 FUZZY matching")
        return
    except Exception:
        pass

    # Fallback: manual fuzzy mapping
    unreal.log("  Performing manual fuzzy chain mapping...")

    source_chains = controller.get_retarget_chains(unreal.RetargetSourceOrTarget.SOURCE)
    target_chains = controller.get_retarget_chains(unreal.RetargetSourceOrTarget.TARGET)

    if not source_chains or not target_chains:
        unreal.log_warning("  No chains found on source or target rig")
        return

    source_names = [chain.chain_name for chain in source_chains] if hasattr(source_chains[0], 'chain_name') else [str(c) for c in source_chains]
    target_names = [chain.chain_name for chain in target_chains] if hasattr(target_chains[0], 'chain_name') else [str(c) for c in target_chains]

    mapped_count = 0
    for source_name in source_names:
        best_match = _fuzzy_match_chain_name(str(source_name), [str(t) for t in target_names])
        if best_match:
            try:
                controller.set_chain_mapping(str(source_name), best_match)
                mapped_count += 1
                unreal.log(f"    Mapped: {source_name} -> {best_match}")
            except Exception as e:
                unreal.log_warning(f"    Failed to map {source_name} -> {best_match}: {e}")

    unreal.log(f"  Fuzzy chain mapping complete: {mapped_count}/{len(source_names)} chains mapped")


def _fuzzy_match_chain_name(source_name, target_names):
    """
    Fuzzy match a source chain name to the best target chain name.
    Chain names are typically like "Spine", "LeftArm", "RightLeg", etc.
    """
    source_lower = source_name.lower().replace("_", "").replace(" ", "")

    # Direct match
    for target in target_names:
        if target.lower().replace("_", "").replace(" ", "") == source_lower:
            return target

    # Keyword-based matching
    body_part_keywords = {
        "spine": ["spine", "torso", "back"],
        "head": ["head", "neck"],
        "leftarm": ["leftarm", "larm", "arml"],
        "rightarm": ["rightarm", "rarm", "armr"],
        "leftleg": ["leftleg", "lleg", "legl"],
        "rightleg": ["rightleg", "rleg", "legr"],
        "lefthand": ["lefthand", "lhand", "handl"],
        "righthand": ["righthand", "rhand", "handr"],
        "leftfoot": ["leftfoot", "lfoot", "footl"],
        "rightfoot": ["rightfoot", "rfoot", "footr"],
    }

    for target in target_names:
        target_lower = target.lower().replace("_", "").replace(" ", "")
        for key, synonyms in body_part_keywords.items():
            if any(s in source_lower for s in synonyms) and any(s in target_lower for s in synonyms):
                return target

    # Partial substring match
    best_match = None
    best_overlap = 0
    for target in target_names:
        target_lower = target.lower().replace("_", "").replace(" ", "")
        # Count shared characters in order (LCS-like)
        overlap = _common_substring_length(source_lower, target_lower)
        if overlap > best_overlap and overlap >= 3:
            best_overlap = overlap
            best_match = target

    return best_match


def _common_substring_length(a, b):
    """Find the length of the longest common substring between two strings."""
    if not a or not b:
        return 0
    m, n = len(a), len(b)
    max_len = 0
    # Simple O(n*m) approach
    prev = [0] * (n + 1)
    for i in range(1, m + 1):
        curr = [0] * (n + 1)
        for j in range(1, n + 1):
            if a[i - 1] == b[j - 1]:
                curr[j] = prev[j - 1] + 1
                max_len = max(max_len, curr[j])
        prev = curr
    return max_len


def list_available_skeletons(content_path="/Game"):
    """
    List all skeleton assets under a content path.
    Useful for finding source/target skeletons for retargeting.
    """
    asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = unreal.EditorAssetLibrary.list_assets(content_path, recursive=True)

    skeletons = []
    for asset_path in assets:
        if "Skeleton" in str(asset_path):
            asset = unreal.EditorAssetLibrary.load_asset(asset_path)
            if isinstance(asset, unreal.Skeleton):
                skeletons.append(asset_path)

    unreal.log(f"Found {len(skeletons)} skeleton(s) under {content_path}")
    for s in skeletons:
        unreal.log(f"  {s}")

    return skeletons

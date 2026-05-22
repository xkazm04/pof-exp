"""
Mixamo One-Click Pipeline
==========================
Complete end-to-end pipeline for importing Mixamo animations into UE5:
  1. Import FBX files with bone prefix stripping
  2. Create IK Rigs for source (Mixamo) and target skeletons
  3. Set up IK Retargeter with fuzzy chain mapping
  4. Batch retarget animations to target skeleton
  5. Extract root motion from in-place animations

Can be run from:
  - UE5 Python console
  - Editor Utility Widget button
  - MixamoImport commandlet (headless)

Usage:
    import mixamo_pipeline
    # Run the full pipeline
    mixamo_pipeline.run_full_pipeline(
        fbx_folder=r"C:\Downloads\Mixamo",
        target_skeleton="/Game/Characters/Player/Meshes/SK_Mannequin",
        output_base="/Game/Characters/Player/Animations/Mixamo"
    )
    # Or step by step
    config = mixamo_pipeline.PipelineConfig()
    config.fbx_folder = r"C:\Downloads\Mixamo"
    config.target_skeleton = "/Game/Characters/Player/Meshes/SK_Mannequin"
    pipeline = mixamo_pipeline.MixamoPipeline(config)
    pipeline.run()
"""

import unreal
import os
import time

# Import our sub-modules
import mixamo_import
import mixamo_retarget
import mixamo_batch_retarget
import mixamo_root_motion


class PipelineConfig:
    """Configuration for the Mixamo import pipeline."""

    def __init__(self):
        # Source settings
        self.fbx_folder = ""                          # Path to folder with Mixamo FBX files
        self.fbx_files = []                            # Specific FBX files (overrides fbx_folder)

        # Import settings
        self.import_destination = "/Game/Characters/Mixamo/Animations"  # Where to import raw anims
        self.import_mesh = True                        # Import skeletal mesh (first file only)
        self.import_skeleton = None                    # Existing skeleton to use (None = create new)

        # Retarget settings
        self.target_skeleton = ""                      # Target skeleton path for retargeting
        self.target_ik_rig = ""                        # Target IK Rig path (empty = auto-create)
        self.retarget_output = "/Game/Characters/Player/Animations/Retargeted"
        self.retarget_suffix = ""                      # Suffix for retargeted anim names (empty = same name)

        # Root motion settings
        self.extract_root_motion = True                # Whether to extract root motion
        self.root_motion_hip_bone = "Hips"             # Hip bone name for root motion extraction
        self.root_motion_forward = True                # Extract forward/back motion
        self.root_motion_lateral = True                # Extract left/right motion
        self.root_motion_vertical = False              # Extract up/down motion
        self.root_motion_filter = None                 # Only extract from animations matching this filter

        # Pipeline settings
        self.skip_import = False                       # Skip FBX import (use existing assets)
        self.skip_retarget = False                     # Skip retargeting
        self.skip_root_motion = False                  # Skip root motion extraction
        self.dry_run = False                           # Log what would happen without doing it

    def validate(self):
        """Validate configuration, returns list of error messages."""
        errors = []

        if not self.skip_import:
            if not self.fbx_folder and not self.fbx_files:
                errors.append("No FBX source specified (set fbx_folder or fbx_files)")
            if self.fbx_folder and not os.path.isdir(self.fbx_folder):
                errors.append(f"FBX folder not found: {self.fbx_folder}")
            for f in self.fbx_files:
                if not os.path.isfile(f):
                    errors.append(f"FBX file not found: {f}")

        if not self.skip_retarget:
            if not self.target_skeleton:
                errors.append("No target skeleton specified for retargeting")

        return errors


class PipelineResult:
    """Results from a pipeline run."""

    def __init__(self):
        self.success = False
        self.start_time = 0
        self.end_time = 0
        self.steps_completed = []
        self.steps_failed = []
        self.imported_assets = []
        self.retargeted_assets = []
        self.root_motion_assets = []
        self.errors = []
        self.warnings = []

    @property
    def elapsed_seconds(self):
        return self.end_time - self.start_time

    def summary(self):
        """Generate a human-readable summary."""
        lines = ["=== Mixamo Pipeline Results ==="]
        lines.append(f"Status: {'SUCCESS' if self.success else 'FAILED'}")
        lines.append(f"Duration: {self.elapsed_seconds:.1f}s")
        lines.append(f"Steps completed: {', '.join(self.steps_completed) or 'None'}")

        if self.steps_failed:
            lines.append(f"Steps failed: {', '.join(self.steps_failed)}")

        lines.append(f"Imported: {len(self.imported_assets)} asset(s)")
        lines.append(f"Retargeted: {len(self.retargeted_assets)} asset(s)")
        lines.append(f"Root motion: {len(self.root_motion_assets)} asset(s)")

        if self.errors:
            lines.append("Errors:")
            for e in self.errors:
                lines.append(f"  - {e}")

        if self.warnings:
            lines.append("Warnings:")
            for w in self.warnings:
                lines.append(f"  - {w}")

        return "\n".join(lines)


class MixamoPipeline:
    """
    Orchestrates the full Mixamo import + retarget + root motion pipeline.
    """

    def __init__(self, config=None):
        self.config = config or PipelineConfig()
        self.result = PipelineResult()
        self._source_ik_rig = None
        self._target_ik_rig = None
        self._retargeter = None

    def run(self):
        """Run the full pipeline based on configuration."""
        self.result = PipelineResult()
        self.result.start_time = time.time()

        unreal.log("=== Starting Mixamo Import Pipeline ===")

        # Validate config
        errors = self.config.validate()
        if errors:
            for e in errors:
                unreal.log_error(f"Config error: {e}")
                self.result.errors.append(e)
            self.result.end_time = time.time()
            return self.result

        if self.config.dry_run:
            unreal.log("[DRY RUN] Pipeline would execute the following steps:")

        try:
            # Step 1: Import FBX files
            if not self.config.skip_import:
                self._step_import()

            # Step 2: Create IK Rigs
            if not self.config.skip_retarget:
                self._step_create_ik_rigs()

            # Step 3: Create Retargeter
            if not self.config.skip_retarget:
                self._step_create_retargeter()

            # Step 4: Batch Retarget
            if not self.config.skip_retarget:
                self._step_batch_retarget()

            # Step 5: Root Motion Extraction
            if not self.config.skip_root_motion and self.config.extract_root_motion:
                self._step_root_motion()

            self.result.success = len(self.result.steps_failed) == 0

        except Exception as e:
            self.result.errors.append(f"Pipeline exception: {str(e)}")
            self.result.success = False

        self.result.end_time = time.time()

        # Print summary
        summary = self.result.summary()
        unreal.log(summary)

        return self.result

    def _step_import(self):
        """Step 1: Import Mixamo FBX files."""
        step_name = "FBX Import"
        unreal.log(f"\n--- Step 1: {step_name} ---")

        if self.config.dry_run:
            unreal.log(f"  [DRY RUN] Would import FBX files to {self.config.import_destination}")
            self.result.steps_completed.append(step_name)
            return

        try:
            if self.config.fbx_files:
                # Import specific files
                for fbx_path in self.config.fbx_files:
                    imported = mixamo_import.import_mixamo_fbx(
                        fbx_path,
                        self.config.import_destination,
                        skeleton=self.config.import_skeleton,
                        as_skeletal_mesh=self.config.import_mesh,
                    )
                    self.result.imported_assets.extend(imported)
                    # After first mesh import, reuse the skeleton
                    if self.config.import_mesh and imported and self.config.import_skeleton is None:
                        self.config.import_mesh = False
            else:
                # Import entire folder
                results = mixamo_import.import_mixamo_folder(
                    self.config.fbx_folder,
                    self.config.import_destination,
                    skeleton=self.config.import_skeleton,
                    as_skeletal_mesh=self.config.import_mesh,
                )
                for filename, paths in results.items():
                    self.result.imported_assets.extend(paths)

            unreal.log(f"  Imported {len(self.result.imported_assets)} asset(s)")
            self.result.steps_completed.append(step_name)

        except Exception as e:
            self.result.steps_failed.append(step_name)
            self.result.errors.append(f"{step_name}: {str(e)}")

    def _step_create_ik_rigs(self):
        """Step 2: Create IK Rigs for source and target skeletons."""
        step_name = "IK Rig Setup"
        unreal.log(f"\n--- Step 2: {step_name} ---")

        if self.config.dry_run:
            unreal.log("  [DRY RUN] Would create IK Rigs for source and target skeletons")
            self.result.steps_completed.append(step_name)
            return

        try:
            # Create source (Mixamo) IK Rig
            # Find the imported skeleton
            source_skeleton = self._find_imported_skeleton()
            if source_skeleton:
                self._source_ik_rig = mixamo_retarget.create_mixamo_ik_rig(
                    source_skeleton,
                    rig_save_path=f"{self.config.import_destination}/IKRig_Mixamo",
                    root_bone=self.config.root_motion_hip_bone,
                )

            # Use or create target IK Rig
            if self.config.target_ik_rig:
                self._target_ik_rig = self.config.target_ik_rig
            else:
                # Auto-create target IK rig if skeleton is provided
                target_parent = "/".join(self.config.target_skeleton.rsplit("/", 1)[:-1])
                self._target_ik_rig = mixamo_retarget.create_mixamo_ik_rig(
                    self.config.target_skeleton,
                    rig_save_path=f"{target_parent}/IKRig_Target",
                    root_bone="pelvis",  # UE5 Mannequin root
                )

            if self._source_ik_rig and self._target_ik_rig:
                self.result.steps_completed.append(step_name)
            else:
                self.result.steps_failed.append(step_name)
                if not self._source_ik_rig:
                    self.result.errors.append("Failed to create source IK Rig")
                if not self._target_ik_rig:
                    self.result.errors.append("Failed to create target IK Rig")

        except Exception as e:
            self.result.steps_failed.append(step_name)
            self.result.errors.append(f"{step_name}: {str(e)}")

    def _step_create_retargeter(self):
        """Step 3: Create IK Retargeter with fuzzy chain mapping."""
        step_name = "Retargeter Setup"
        unreal.log(f"\n--- Step 3: {step_name} ---")

        if self.config.dry_run:
            unreal.log("  [DRY RUN] Would create IK Retargeter with fuzzy mapping")
            self.result.steps_completed.append(step_name)
            return

        if not self._source_ik_rig or not self._target_ik_rig:
            self.result.steps_failed.append(step_name)
            self.result.errors.append("Missing IK Rigs for retargeter creation")
            return

        try:
            self._retargeter = mixamo_retarget.create_retargeter(
                source_ik_rig=self._source_ik_rig,
                target_ik_rig=self._target_ik_rig,
                save_path=f"{self.config.import_destination}/RTG_Mixamo_To_Target",
                auto_map=True,
            )

            if self._retargeter:
                self.result.steps_completed.append(step_name)
            else:
                self.result.steps_failed.append(step_name)
                self.result.errors.append("Failed to create IK Retargeter")

        except Exception as e:
            self.result.steps_failed.append(step_name)
            self.result.errors.append(f"{step_name}: {str(e)}")

    def _step_batch_retarget(self):
        """Step 4: Batch retarget all imported animations."""
        step_name = "Batch Retarget"
        unreal.log(f"\n--- Step 4: {step_name} ---")

        if self.config.dry_run:
            unreal.log(f"  [DRY RUN] Would retarget {len(self.result.imported_assets)} animation(s)")
            self.result.steps_completed.append(step_name)
            return

        if not self._retargeter:
            self.result.steps_failed.append(step_name)
            self.result.errors.append("No retargeter available for batch retarget")
            return

        try:
            results = mixamo_batch_retarget.batch_retarget(
                retargeter_path=self._retargeter,
                source_folder=self.config.import_destination,
                output_folder=self.config.retarget_output,
                name_suffix=self.config.retarget_suffix,
            )

            for source, output in results.items():
                if not str(output).startswith("ERROR"):
                    self.result.retargeted_assets.append(output)
                else:
                    self.result.warnings.append(f"Retarget failed for {source}: {output}")

            self.result.steps_completed.append(step_name)

        except Exception as e:
            self.result.steps_failed.append(step_name)
            self.result.errors.append(f"{step_name}: {str(e)}")

    def _step_root_motion(self):
        """Step 5: Extract root motion from retargeted animations."""
        step_name = "Root Motion"
        unreal.log(f"\n--- Step 5: {step_name} ---")

        # Determine which folder to process
        target_folder = self.config.retarget_output if not self.config.skip_retarget else self.config.import_destination

        if self.config.dry_run:
            unreal.log(f"  [DRY RUN] Would extract root motion from animations in {target_folder}")
            self.result.steps_completed.append(step_name)
            return

        try:
            results = mixamo_root_motion.batch_extract_root_motion(
                folder_path=target_folder,
                hip_bone=self.config.root_motion_hip_bone,
                extract_forward=self.config.root_motion_forward,
                extract_lateral=self.config.root_motion_lateral,
                extract_vertical=self.config.root_motion_vertical,
                name_filter=self.config.root_motion_filter,
            )

            for anim_path, status in results.items():
                if status == "OK":
                    self.result.root_motion_assets.append(anim_path)
                else:
                    self.result.warnings.append(f"Root motion extraction failed: {anim_path}")

            self.result.steps_completed.append(step_name)

        except Exception as e:
            self.result.steps_failed.append(step_name)
            self.result.errors.append(f"{step_name}: {str(e)}")

    def _find_imported_skeleton(self):
        """Find the skeleton from imported assets."""
        # Search imported assets for a skeleton
        for asset_path in self.result.imported_assets:
            asset = unreal.EditorAssetLibrary.load_asset(asset_path)
            if isinstance(asset, unreal.Skeleton):
                return asset_path
            elif isinstance(asset, unreal.SkeletalMesh):
                skeleton = asset.get_editor_property("skeleton")
                if skeleton:
                    return skeleton.get_path_name()

        # Fallback: search the import destination
        skeletons = mixamo_retarget.list_available_skeletons(self.config.import_destination)
        if skeletons:
            return skeletons[0]

        self.result.warnings.append("Could not find imported Mixamo skeleton")
        return None


# --- Convenience functions for quick usage ---

def run_full_pipeline(fbx_folder, target_skeleton, output_base="/Game/Characters/Player/Animations/Mixamo",
                       extract_root_motion=True, hip_bone="Hips"):
    """
    One-function call to run the entire Mixamo pipeline.

    Args:
        fbx_folder: Path to folder containing Mixamo FBX files.
        target_skeleton: Content path to target skeleton for retargeting.
        output_base: Base content path for all output assets.
        extract_root_motion: Whether to extract root motion.
        hip_bone: Hip bone name for root motion extraction.

    Returns:
        PipelineResult with full details.
    """
    config = PipelineConfig()
    config.fbx_folder = fbx_folder
    config.target_skeleton = target_skeleton
    config.import_destination = f"{output_base}/Source"
    config.retarget_output = f"{output_base}/Retargeted"
    config.extract_root_motion = extract_root_motion
    config.root_motion_hip_bone = hip_bone

    pipeline = MixamoPipeline(config)
    return pipeline.run()


def import_only(fbx_folder, destination="/Game/Characters/Mixamo/Animations", import_mesh=True):
    """Just import Mixamo FBX files (no retarget, no root motion)."""
    config = PipelineConfig()
    config.fbx_folder = fbx_folder
    config.import_destination = destination
    config.import_mesh = import_mesh
    config.skip_retarget = True
    config.skip_root_motion = True

    pipeline = MixamoPipeline(config)
    return pipeline.run()


def retarget_only(source_folder, target_skeleton, output_folder,
                   source_ik_rig=None, target_ik_rig=None):
    """Just retarget existing animations (no import, no root motion)."""
    config = PipelineConfig()
    config.import_destination = source_folder
    config.target_skeleton = target_skeleton
    config.target_ik_rig = target_ik_rig or ""
    config.retarget_output = output_folder
    config.skip_import = True
    config.skip_root_motion = True

    pipeline = MixamoPipeline(config)
    return pipeline.run()


def root_motion_only(anim_folder, hip_bone="Hips", name_filter=None):
    """Just extract root motion from existing animations."""
    config = PipelineConfig()
    config.import_destination = anim_folder
    config.root_motion_hip_bone = hip_bone
    config.root_motion_filter = name_filter
    config.skip_import = True
    config.skip_retarget = True

    pipeline = MixamoPipeline(config)
    return pipeline.run()

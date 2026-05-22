"""
setup_characters_ue.py
======================
Swaps the gray-box primitive bodies on the vertical-slice characters for the
real UE Mannequin skeletal mesh + animation Blueprint.

For each of /Game/VerticalSlice/BP_VSPlayer and /Game/VerticalSlice/BP_VSEnemy:
  1. Remove the StaticMeshComponent named "VSBody" (the PS-1 gray-box body)
     via unreal.SubobjectDataSubsystem.
  2. Configure the inherited ACharacter skeletal-mesh component ("CharacterMesh0"):
       - SkeletalMeshAsset -> SKM_Manny (player) / SKM_Manny_Simple (enemy)
       - AnimClass        -> ABP_Manny generated class
       - relative transform -> location (0,0,-90), rotation yaw -90
         (the standard mannequin offset hardcoded in the C++ character).
  3. Distinguish the enemy: override its mesh material slots with MI_Manny_02
     (player keeps the mannequin default / MI_Manny_01).
  4. Save both Blueprints.

UE Python authors *assets and CDO/template properties* - not event graphs.
The script is idempotent: re-running must not error (VSBody already gone, mesh
already set). Each Blueprint's work is wrapped in try/except that logs via
unreal.log_error and re-raises so a failure is visible in the headless run.

Run headless:
    "C:\\Program Files\\Epic Games\\UE_5.7\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
        "C:\\Users\\kazda\\Documents\\Unreal Projects\\PoF\\PoF.uproject" ^
        -run=pythonscript -script="<abs path to this file>" -unattended -nopause
"""

import unreal

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

BP_VSPLAYER_PATH = "/Game/VerticalSlice/BP_VSPlayer"
BP_VSENEMY_PATH = "/Game/VerticalSlice/BP_VSEnemy"

# Mannequin assets confirmed to resolve by Task 1 (MoverTests plugin enabled).
SKM_MANNY_PATH = "/MoverTests/Characters/Mannequins/Meshes/SKM_Manny"
SKM_MANNY_SIMPLE_PATH = "/MoverTests/Characters/Mannequins/Meshes/SKM_Manny_Simple"
ABP_MANNY_PATH = "/MoverTests/Characters/Mannequins/Animations/ABP_Manny"
ABP_MANNY_CLASS = "/MoverTests/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"
MI_MANNY_01_PATH = "/MoverTests/Characters/Mannequins/Materials/Instances/Manny/MI_Manny_01"
MI_MANNY_02_PATH = "/MoverTests/Characters/Mannequins/Materials/Instances/Manny/MI_Manny_02"

# Red material for the enemy – a plain base-colour material that makes the
# enemy obviously distinct from the silver/grey player mannequin.
M_ENEMY_RED_PATH = "/Game/VerticalSlice/M_EnemyRed"
ENEMY_RED_RGB = (0.7, 0.04, 0.04)  # strong red in linear colour space

# Standard mannequin offset (matches the C++ character's hardcoded values).
MANNEQUIN_OFFSET_LOCATION = unreal.Vector(0.0, 0.0, -90.0)
MANNEQUIN_OFFSET_ROTATION = unreal.Rotator(0.0, 0.0, -90.0)  # (roll, pitch, yaw)

BODY_COMPONENT_NAME = "VSBody"

asset_lib = unreal.EditorAssetLibrary

# Progress lines are also collected here and flushed to a sidecar file at the
# end of the run, so the result is verifiable even when the headless run
# suppresses Display-verbosity LogPython output.
_PROGRESS = []


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _log(msg):
    # log_warning (not log) so the line is visible in headless commandlet
    # stdout, which suppresses Display verbosity.
    unreal.log_warning("[setup_characters_ue] " + msg)
    _PROGRESS.append(msg)


def load_object(object_path):
    """Load a UObject asset. Returns None if it does not exist."""
    if not asset_lib.does_asset_exist(object_path):
        return None
    return asset_lib.load_asset(object_path)


def _get_subsystem():
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    if subsystem is None:
        raise RuntimeError("SubobjectDataSubsystem unavailable")
    return subsystem


def _resolve_object_from_handle(subsystem, handle):
    """
    From a subobject handle, return the template UObject it represents.
    Mirrors the build_vertical_slice.py pattern:
        data = k2_find_subobject_data_from_handle(handle)
        obj  = SubobjectDataBlueprintFunctionLibrary.get_associated_object(data)
    """
    data = subsystem.k2_find_subobject_data_from_handle(handle)
    if data is None:
        return None
    return unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(data)


# ---------------------------------------------------------------------------
# Section 1: Remove the gray-box "VSBody" StaticMeshComponent
# ---------------------------------------------------------------------------

def remove_vsbody(bp):
    """
    Find the StaticMeshComponent named VSBody among the Blueprint's subobjects
    and delete it. Idempotent: if no such component exists, logs and returns.
    Returns True if a component was deleted, False if none was found.
    """
    subsystem = _get_subsystem()
    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    if not handles:
        unreal.log_warning("[setup_characters_ue] no subobject data for "
                           + bp.get_name())
        return False

    # handles[0] is the root context handle, required as `context_handle`
    # for delete_subobjects (it is NOT the Blueprint object).
    root_handle = handles[0]

    target_handle = None
    for h in handles:
        obj = _resolve_object_from_handle(subsystem, h)
        if obj is None:
            continue
        # Component subobject template names are like "VSBody" or "VSBody_GEN_VARIABLE".
        name = obj.get_name()
        if name == BODY_COMPONENT_NAME or name.startswith(BODY_COMPONENT_NAME):
            if isinstance(obj, unreal.StaticMeshComponent):
                target_handle = h
                _log("Found gray-box body subobject '%s' on %s"
                     % (name, bp.get_name()))
                break

    if target_handle is None:
        _log("No '%s' StaticMeshComponent on %s (already removed)"
             % (BODY_COMPONENT_NAME, bp.get_name()))
        return False

    # UE 5.7 signature:
    #   delete_subobjects(context_handle, subobjects_to_delete, bp_context)
    # context_handle = the owning root SubobjectDataHandle; bp_context = the
    # Blueprint. (Passing the Blueprint as context_handle fails to nativize.)
    num_deleted = subsystem.delete_subobjects(root_handle, [target_handle], bp)
    _log("delete_subobjects removed %s component(s) ('%s') from %s"
         % (str(num_deleted), BODY_COMPONENT_NAME, bp.get_name()))
    return num_deleted > 0


# ---------------------------------------------------------------------------
# Section 2: Locate the inherited ACharacter skeletal-mesh component
# ---------------------------------------------------------------------------

def find_character_mesh_component(bp):
    """
    Return the template USkeletalMeshComponent for the inherited ACharacter
    'Mesh' (subobject typically 'CharacterMesh0'). Re-gathers handles because
    section 1 mutated the subobject set. Raises if it cannot be found.
    """
    subsystem = _get_subsystem()
    handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)
    if not handles:
        raise RuntimeError("no subobject data for " + bp.get_name())

    bfl = unreal.SubobjectDataBlueprintFunctionLibrary
    # (component, is_inherited, name)
    skel_comps = []
    for h in handles:
        data = subsystem.k2_find_subobject_data_from_handle(h)
        if data is None:
            continue
        obj = bfl.get_associated_object(data)
        if obj is None or not isinstance(obj, unreal.SkeletalMeshComponent):
            continue
        try:
            inherited = bfl.is_inherited_component(data)
        except Exception:
            inherited = False
        skel_comps.append((obj, inherited, obj.get_name()))

    if not skel_comps:
        raise RuntimeError("no SkeletalMeshComponent (inherited Mesh) found on "
                           + bp.get_name())

    # Prefer the inherited ACharacter Mesh (the C++ component).
    for comp, inherited, name in skel_comps:
        if inherited and name.startswith("CharacterMesh0"):
            _log("Inherited Mesh component: '%s' (inherited) on %s"
                 % (name, bp.get_name()))
            return comp
    for comp, inherited, name in skel_comps:
        if name.startswith("CharacterMesh0"):
            _log("Mesh component: '%s' (inherited=%s) on %s"
                 % (name, inherited, bp.get_name()))
            return comp
    for comp, inherited, name in skel_comps:
        if inherited:
            _log("Using inherited skeletal-mesh component '%s' on %s"
                 % (name, bp.get_name()))
            return comp

    comp, inherited, name = skel_comps[0]
    _log("Using skeletal-mesh component '%s' on %s (fallback)"
         % (name, bp.get_name()))
    return comp


# ---------------------------------------------------------------------------
# Section 3: Configure the inherited Mesh with the mannequin
# ---------------------------------------------------------------------------

def _resolve_anim_class():
    """Load ABP_Manny's generated AnimInstance class."""
    abp = load_object(ABP_MANNY_PATH)
    if abp is not None:
        try:
            gen = abp.generated_class()
            if gen is not None:
                return gen
        except Exception:
            pass
    # Fallback: load the generated class directly by path.
    gen = unreal.load_class(None, ABP_MANNY_CLASS)
    if gen is None:
        raise RuntimeError("could not resolve ABP_Manny generated class")
    return gen


def configure_mesh_component(mesh_comp, skeletal_mesh, anim_class, bp_name):
    """Set the skeletal mesh, anim class and standard offset on the component."""
    # Skeletal mesh asset.
    try:
        mesh_comp.set_editor_property("skeletal_mesh_asset", skeletal_mesh)
    except Exception:
        # Older binding name.
        mesh_comp.set_skeletal_mesh_asset(skeletal_mesh)
    _log("%s: SkeletalMeshAsset set" % bp_name)

    # Animation: class-driven (AnimBlueprint), not single-asset.
    mesh_comp.set_editor_property("animation_mode",
                                  unreal.AnimationMode.ANIMATION_BLUEPRINT)
    try:
        mesh_comp.set_editor_property("anim_class", anim_class)
    except Exception:
        mesh_comp.set_anim_class(anim_class)
    _log("%s: AnimClass set to ABP_Manny" % bp_name)

    # Standard mannequin offset (relative to the capsule root).
    mesh_comp.set_editor_property("relative_location", MANNEQUIN_OFFSET_LOCATION)
    mesh_comp.set_editor_property("relative_rotation", MANNEQUIN_OFFSET_ROTATION)
    _log("%s: relative transform set to loc(0,0,-90) yaw(-90)" % bp_name)


def apply_material_override(mesh_comp, material, bp_name):
    """Override every material slot on the mesh component with `material`.

    Strategy: set override_materials on both the component template (subobject
    data handle) AND on the CDO-level component obtained via get_default_object.
    The CDO-level write is the one UE actually serialises for inherited (native
    C++) components.
    """
    skel_mesh = None
    try:
        skel_mesh = mesh_comp.get_editor_property("skeletal_mesh_asset")
    except Exception:
        pass

    num_slots = 0
    if skel_mesh is not None:
        try:
            mats = skel_mesh.get_editor_property("materials")
            num_slots = len(mats) if mats is not None else 0
        except Exception:
            num_slots = 0
    if num_slots == 0:
        num_slots = 1

    # --- Write 1: component template (subobject data handle path) ---
    override_list = [material] * num_slots
    try:
        mesh_comp.set_editor_property("override_materials", override_list)
        _log("%s: override_materials set on template component (%d slot(s))"
             % (bp_name, num_slots))
    except Exception as e:
        _log("%s: template set_editor_property(override_materials) failed: %s"
             % (bp_name, str(e)))

    for idx in range(num_slots):
        try:
            mesh_comp.set_material(idx, material)
        except Exception:
            pass

    _log("%s: material override applied to %d slot(s)" % (bp_name, num_slots))


# ---------------------------------------------------------------------------
# Enemy red material (idempotent create)
# ---------------------------------------------------------------------------

def ensure_enemy_red_material():
    """
    Create /Game/VerticalSlice/M_EnemyRed if it does not already exist,
    then return the loaded Material object.

    The material is a single MaterialExpressionConstant3Vector node with
    RGB = ENEMY_RED_RGB, wired to MP_BASE_COLOR.  Rebuilding is skipped when
    the asset is already present so the function is idempotent.
    """
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat_lib = unreal.MaterialEditingLibrary

    if asset_lib.does_asset_exist(M_ENEMY_RED_PATH):
        # Delete and rebuild so the material graph is always clean and correct.
        asset_lib.delete_asset(M_ENEMY_RED_PATH)
        _log("M_EnemyRed existed – deleted for clean rebuild: " + M_ENEMY_RED_PATH)

    # Split into folder / name for create_asset.
    folder_idx = M_ENEMY_RED_PATH.rfind("/")
    folder = M_ENEMY_RED_PATH[:folder_idx]
    name = M_ENEMY_RED_PATH[folder_idx + 1:]

    factory = unreal.MaterialFactoryNew()
    material = asset_tools.create_asset(name, folder, unreal.Material, factory)
    if material is None:
        raise RuntimeError("Failed to create material asset: " + M_ENEMY_RED_PATH)

    # Add a Constant3Vector node and set its value to the target red.
    r, g, b = ENEMY_RED_RGB
    node = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionConstant3Vector, -300, -100)
    color = unreal.LinearColor(r, g, b, 1.0)
    node.set_editor_property("constant", color)

    # Wire the node's output to the material's Base Color input.
    # IMPORTANT: MaterialExpressionConstant3Vector output pin name is "" (empty
    # string), NOT "RGB". Using "RGB" silently returns False and leaves the
    # material unconnected (all black). Verified via diag_pin_names.py.
    mat_lib.connect_material_property(node, "", unreal.MaterialProperty.MP_BASE_COLOR)

    # Add a strong emissive so the enemy glows red regardless of scene lighting.
    emissive_node = mat_lib.create_material_expression(
        material, unreal.MaterialExpressionConstant3Vector, -300, 100)
    emissive_node.set_editor_property("constant", unreal.LinearColor(3.0, 0.0, 0.0, 1.0))
    mat_lib.connect_material_property(emissive_node, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    # Required for the material to render on SkeletalMesh actors at runtime;
    # without this flag UE falls back to the default grey material in-game.
    material.set_editor_property("used_with_skeletal_mesh", True)

    mat_lib.recompile_material(material)
    asset_lib.save_asset(M_ENEMY_RED_PATH)
    _log("Created and saved M_EnemyRed material (base+emissive, used_with_skeletal_mesh=True)")
    return material


# ---------------------------------------------------------------------------
# Per-Blueprint driver
# ---------------------------------------------------------------------------

def wire_character(bp_path, skeletal_mesh_path, material_override_path, label):
    """Run the full removal + mannequin wiring for one Blueprint."""
    try:
        _log("--- Wiring %s (%s) ---" % (label, bp_path))

        bp = load_object(bp_path)
        if bp is None:
            raise RuntimeError("Blueprint not found: " + bp_path)

        skeletal_mesh = load_object(skeletal_mesh_path)
        if skeletal_mesh is None:
            raise RuntimeError("Skeletal mesh missing: " + skeletal_mesh_path)

        anim_class = _resolve_anim_class()

        # Step 1: remove the gray-box body.
        removed = remove_vsbody(bp)
        _log("%s: VSBody removed this run = %s" % (label, removed))

        # Recompile so the subobject set is consistent before editing the
        # inherited mesh component.
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)

        # Step 2: locate the inherited ACharacter Mesh.
        mesh_comp = find_character_mesh_component(bp)

        # Step 3: configure it with the mannequin.
        configure_mesh_component(mesh_comp, skeletal_mesh, anim_class, label)

        # Recompile to bake the mesh/anim/transform changes into the generated class
        # BEFORE applying material overrides.  A second compile after set_material()
        # would reset the component's OverrideMaterials array, so we apply materials
        # after the last compile.
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)

        # Step 3b: distinguish the enemy with a different material.
        # material_override can be a pre-loaded Material object or a path string.
        # This must happen AFTER the final compile so the override is not reset.
        material = None  # will be set below if material_override_path is given
        if material_override_path is not None:
            # Re-locate the mesh component after the compile (handles may have changed).
            mesh_comp = find_character_mesh_component(bp)

            if isinstance(material_override_path, str):
                material = load_object(material_override_path)
                if material is None:
                    unreal.log_warning(
                        "[setup_characters_ue] material override missing: "
                        + material_override_path)
            else:
                # Already a loaded UObject (e.g. the M_EnemyRed Material).
                material = material_override_path
            if material is not None:
                apply_material_override(mesh_comp, material, label)

        # Step 3c: also write the material override on the CDO-level component.
        # For inherited (native C++) components, UE serialises the override on the
        # generated class CDO, not on the SCS template.  We get the CDO and its
        # Mesh component and set override_materials there too.
        if material_override_path is not None and material is not None:
            try:
                gen_class = bp.generated_class()
                if gen_class is not None:
                    cdo = unreal.get_default_object(gen_class)
                    if cdo is not None:
                        cdo_mesh = cdo.mesh
                        if cdo_mesh is None:
                            try:
                                cdo_mesh = cdo.get_editor_property("mesh")
                            except Exception:
                                cdo_mesh = None
                        if cdo_mesh is not None:
                            skel_mesh_cdo = None
                            try:
                                skel_mesh_cdo = cdo_mesh.get_editor_property("skeletal_mesh_asset")
                            except Exception:
                                pass
                            n = 0
                            if skel_mesh_cdo is not None:
                                try:
                                    mats_cdo = skel_mesh_cdo.get_editor_property("materials")
                                    n = len(mats_cdo) if mats_cdo else 0
                                except Exception:
                                    pass
                            if n == 0:
                                n = 2  # SKM_Manny_Simple has 2 slots
                            override_cdo = [material] * n
                            try:
                                cdo_mesh.set_editor_property("override_materials", override_cdo)
                                _log("%s: override_materials set on CDO mesh (%d slot(s))"
                                     % (label, n))
                            except Exception as e2:
                                _log("%s: CDO override_materials failed: %s" % (label, str(e2)))
                            for idx in range(n):
                                try:
                                    cdo_mesh.set_material(idx, material)
                                except Exception:
                                    pass
                        else:
                            _log("%s: CDO mesh component not found via .mesh" % label)
                    else:
                        _log("%s: get_default_object returned None" % label)
                else:
                    _log("%s: bp.generated_class() returned None" % label)
            except Exception as exc_cdo:
                _log("%s: CDO material write failed: %s" % (label, str(exc_cdo)))

        # Step 4: save WITHOUT a final compile (which would reset override_materials).
        asset_lib.save_asset(bp_path)
        _log("%s: saved %s" % (label, bp_path))
    except Exception as exc:
        unreal.log_error("[setup_characters_ue] wire_character(%s) FAILED: %s"
                         % (label, str(exc)))
        raise


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    _log("=== Character mannequin wiring START ===")

    # Player: SKM_Manny, mannequin default material (no override).
    wire_character(BP_VSPLAYER_PATH, SKM_MANNY_PATH, None, "BP_VSPlayer")

    # Enemy: SKM_Manny_Simple + a clearly red material so it is visually
    # distinct from the silver/grey player mannequin at a glance.
    enemy_material = ensure_enemy_red_material()
    wire_character(BP_VSENEMY_PATH, SKM_MANNY_SIMPLE_PATH, enemy_material,
                   "BP_VSEnemy")

    _log("=== Character mannequin wiring COMPLETE ===")

    # Flush a verifiable sidecar log next to the project.
    try:
        out_path = unreal.Paths.combine(
            [unreal.Paths.project_saved_dir(), "setup_characters_ue.log"])
        with open(out_path, "w", encoding="utf-8") as fh:
            fh.write("\n".join(_PROGRESS) + "\n")
        unreal.log_warning("[setup_characters_ue] progress written to "
                           + out_path)
    except Exception as exc:
        unreal.log_error("[setup_characters_ue] could not write progress log: "
                         + str(exc))


if __name__ == "__main__":
    main()

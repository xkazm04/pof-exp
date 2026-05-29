"""Step 06 — Author BS_Locomotion's 8-way strafe sample grid (pure Python, UE 5.7).

There is no BlendSpaceLibrary.add_sample in 5.7, so we author the BlendSpace
directly: set its skeleton, configure the two blend axes (Direction X, Speed Y),
and assign the sample_data array of FBlendSample. Setting sample_data fires
PostEditChangeProperty, which rebuilds the internal blend grid.

Axes: X = direction (-1 left .. +1 right), Y = speed (-1 back .. +1 run).
"""

import unreal


BS_PATH = "/Game/Characters/Player/Animations/BS_Locomotion"
RT_DIR = "/Game/Mixamo/Retargeted/SKM_Manny"
MANNY_SKEL_CANDIDATES = [
    "/MoverTests/Characters/Mannequins/Meshes/SK_Mannequin",
    "/MoverExamples/Characters/Mannequins/Meshes/SK_Mannequin",
]

# Conventional locomotion blend space driven by UARPGAnimInstance:
#   X = Direction (-180..180, movement dir relative to facing), Y = Speed (cm/s).
# (Direction°, Speed cm/s, retargeted clip base name — without _RT)
WALK = 200.0
RUN = 450.0
GRID = [
    (0.0, 0.0, "Standard_Idle"),
    # Walk ring
    (0.0, WALK, "Walking"),
    (90.0, WALK, "Right_Strafe_Walking"),
    (-90.0, WALK, "Left_Strafe_Walking"),
    (180.0, WALK, "Walking_Backwards"),
    (-180.0, WALK, "Walking_Backwards"),
    # Run ring
    (0.0, RUN, "Running"),
    (90.0, RUN, "Right_Strafe"),
    (-90.0, RUN, "Left_Strafe"),
    (180.0, RUN, "Running_Backward"),
    (-180.0, RUN, "Running_Backward"),
]


def _axis(name, lo, hi, grid_num):
    p = unreal.BlendParameter()
    p.set_editor_property("display_name", name)
    p.set_editor_property("min", lo)
    p.set_editor_property("max", hi)
    p.set_editor_property("grid_num", grid_num)
    return p


def _manny_skeleton():
    for p in MANNY_SKEL_CANDIDATES:
        s = unreal.EditorAssetLibrary.load_asset(p)
        if s:
            return s
    return None


def run(args):
    result = {"created": [], "skipped": [], "failed": [], "sample_count": 0}

    skel = _manny_skeleton()
    if not skel:
        result["failed"].append("Manny skeleton not found")
        return result

    # The BlendSpace skeleton can't be set after creation (read-only) and isn't
    # inferred from a direct sample_data assignment. So (re)create BS_Locomotion via
    # BlendSpaceFactoryNew with the target skeleton — otherwise the AnimBP's
    # BlendSpacePlayer fails to compile ("references Blend Space with missing skeleton").
    bs = unreal.EditorAssetLibrary.load_asset(BS_PATH)
    needs_recreate = (not bs) or (bs.get_skeleton() is None)
    if needs_recreate:
        if unreal.EditorAssetLibrary.does_asset_exist(BS_PATH):
            unreal.EditorAssetLibrary.delete_asset(BS_PATH)
        pkg_dir, name = BS_PATH.rsplit("/", 1)
        factory = unreal.BlendSpaceFactoryNew()
        factory.set_editor_property("target_skeleton", skel)
        bs = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, pkg_dir, unreal.BlendSpace, factory)
        if not bs:
            result["failed"].append("create_asset(BlendSpace) failed")
            return result

    # Configure the two blend axes (X=Direction, Y=Speed); leave Z default.
    try:
        params = list(bs.get_editor_property("blend_parameters"))
        params[0] = _axis("Direction", -180.0, 180.0, 4)
        params[1] = _axis("Speed", 0.0, RUN, 2)
        bs.set_editor_property("blend_parameters", params)
    except Exception as e:  # noqa: BLE001
        result["failed"].append(f"set blend_parameters: {e}")

    # Build the sample array.
    samples = []
    for x, y, base in GRID:
        anim = unreal.EditorAssetLibrary.load_asset(f"{RT_DIR}/{base}_RT")
        if not anim:
            result["failed"].append(f"missing retargeted clip: {base}_RT")
            continue
        s = unreal.BlendSample()
        s.set_editor_property("animation", anim)
        s.set_editor_property("sample_value", unreal.Vector(x, y, 0.0))
        s.set_editor_property("rate_scale", 1.0)
        samples.append(s)
        result["created"].append(f"{base}@({x},{y})")

    try:
        bs.set_editor_property("sample_data", samples)
        unreal.EditorAssetLibrary.save_asset(BS_PATH)
    except Exception as e:  # noqa: BLE001
        result["failed"].append(f"set sample_data: {e}")

    try:
        result["sample_count"] = len(bs.get_editor_property("sample_data"))
    except Exception:  # noqa: BLE001
        result["sample_count"] = len(samples)
    return result

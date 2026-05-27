"""Step 06 — Wire the 11 retargeted clips into BS_Locomotion's sample grid.

The BS axes are X=direction (-1 left .. +1 right), Y=speed (-1 backward .. +1 run).
The grid layout (positive-Y is forward motion; Idle occupies the three zero-speed
positions):

    (-1,  1) Left_Strafe       (0,  1) Running          (1,  1) Right_Strafe
    (-1, 0.5) Left_Strafe_Walk (0, 0.5) Walking         (1, 0.5) Right_Strafe_Walk
    (-1,  0) Idle              (0,  0) Idle             (1,  0) Idle
                               (0,-0.5) Walking_Backwards
                               (0, -1)  Running_Backward
"""

import unreal


BS_PATH = "/Game/Characters/Player/Animations/BS_Locomotion"
RT_DIR  = "/Game/Mixamo/Retargeted/SKM_Manny"


# (X, Y, retargeted_clip_basename — without `_RT` suffix)
GRID = [
    (-1.0,  0.0, "Standard_Idle"),
    ( 0.0,  0.0, "Standard_Idle"),
    ( 1.0,  0.0, "Standard_Idle"),
    (-1.0,  0.5, "Left_Strafe_Walking"),
    ( 0.0,  0.5, "Walking"),
    ( 1.0,  0.5, "Right_Strafe_Walking"),
    (-1.0,  1.0, "Left_Strafe"),
    ( 0.0,  1.0, "Running"),
    ( 1.0,  1.0, "Right_Strafe"),
    ( 0.0, -0.5, "Walking_Backwards"),
    ( 0.0, -1.0, "Running_Backward"),
]


def _clear_samples(bs):
    """Best-effort sample-clear so re-runs produce a deterministic grid."""
    try:
        existing = unreal.BlendSpaceLibrary.get_sample_count(bs)
        for _ in range(existing):
            unreal.BlendSpaceLibrary.remove_sample(bs, 0)
    except Exception:
        # API may not be available in every UE version; if not, we just append
        # and tolerate duplicates (the grid is deterministic enough that adding
        # again is harmless).
        pass


def run(args):
    result = {"created": [], "skipped": [], "failed": [], "sample_count": 0}
    bs = unreal.EditorAssetLibrary.load_asset(BS_PATH)
    if not bs:
        result["failed"].append(f"BS_Locomotion not found at {BS_PATH}")
        return result

    _clear_samples(bs)

    for x, y, base in GRID:
        anim_path = f"{RT_DIR}/{base}_RT"
        anim = unreal.EditorAssetLibrary.load_asset(anim_path)
        if not anim:
            result["failed"].append(f"missing retargeted clip: {anim_path}")
            continue
        try:
            unreal.BlendSpaceLibrary.add_sample(bs, anim, unreal.Vector(x, y, 0))
            result["created"].append(f"{base}@({x},{y})")
        except Exception as e:
            result["failed"].append(f"add_sample {base}@({x},{y}): {e}")

    unreal.EditorAssetLibrary.save_asset(BS_PATH)
    try:
        result["sample_count"] = unreal.BlendSpaceLibrary.get_sample_count(bs)
    except Exception:
        result["sample_count"] = len(result["created"])
    return result

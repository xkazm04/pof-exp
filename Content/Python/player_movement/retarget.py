"""Step 05 — Batch retarget Mixamo clips to the Manny skeleton.

Reads `/Game/Mixamo/Raw/`, retargets every AnimSequence to
`/Game/Mixamo/Retargeted/SKM_Manny/<Name>_RT` using RTG_MixamoToManny. Idempotent.
"""

import unreal


SRC_DIR = "/Game/Mixamo/Raw"
DST_DIR = "/Game/Mixamo/Retargeted/SKM_Manny"
RETARGETER_PATH = "/Game/Characters/Player/IK/RTG_MixamoToManny"


def _list_source_clips():
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = ar.get_assets_by_path(SRC_DIR, recursive=True)
    names = []
    for a in assets:
        obj_path = str(a.object_path) if hasattr(a, "object_path") else str(a.package_name)
        name = obj_path.rsplit("/", 1)[-1].split(".")[0]
        if name.endswith("_Skeleton") or name.endswith("_PhysicsAsset") or name.endswith("_Skin"):
            continue
        # Only include AnimSequences (skip the mesh-bearing asset)
        if hasattr(a, "asset_class_path"):
            cls_name = str(a.asset_class_path.asset_name)
        else:
            cls_name = str(getattr(a, "asset_class", ""))
        if "AnimSequence" not in cls_name and "Animation" not in cls_name:
            continue
        names.append(name)
    return names


def run(args):
    result = {"created": [], "skipped": [], "failed": []}
    try:
        rtg = unreal.EditorAssetLibrary.load_asset(RETARGETER_PATH)
        if not rtg:
            result["failed"].append(f"retargeter missing: {RETARGETER_PATH}; run step 04 first")
            return result

        candidates = _list_source_clips()
        to_retarget = []
        for name in candidates:
            target = f"{DST_DIR}/{name}_RT"
            if unreal.EditorAssetLibrary.does_asset_exist(target):
                result["skipped"].append(name)
            else:
                to_retarget.append(name)

        if to_retarget:
            ctrl = unreal.IKRetargeterController.get_controller(rtg)
            # batch_retarget_animations signature varies by UE version; try the modern shape first
            try:
                outputs = ctrl.batch_retarget_animations([
                    {"source_animation": f"{SRC_DIR}/{n}", "destination_folder": DST_DIR}
                    for n in to_retarget
                ])
            except Exception:
                # Fallback: per-clip retarget
                outputs = []
                for n in to_retarget:
                    try:
                        out = ctrl.retarget_animations(
                            [unreal.EditorAssetLibrary.load_asset(f"{SRC_DIR}/{n}")],
                            DST_DIR
                        )
                        outputs.extend(out or [])
                    except Exception as inner:
                        result["failed"].append(f"retarget {n}: {inner}")

            for out in outputs:
                # outputs are AnimSequence assets; their asset name should end with _RT or be the new name
                try:
                    path = str(out.get_path_name()) if hasattr(out, "get_path_name") else str(out)
                except Exception:
                    path = str(out)
                name = path.rsplit("/", 1)[-1].split(".")[0]
                if name not in result["created"]:
                    result["created"].append(name)
    except Exception as e:
        result["failed"].append(str(e))

    return result

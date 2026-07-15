"""Retarget ARDY-generated clips (slash/run/roll/idle) onto the UE5 Manny.

Self-contained, commandlet-runnable:
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=Content/Python/ardy_retarget.py -nullrhi

Steps:
 1. Re-import run/roll/idle FBX as ANIM-ONLY onto slash's skeleton (each clip imported
    with its own skeleton originally; the retarget batch needs ONE source skeleton).
 2. Build IK_ArdyCore (auto template; manual Mixamo-style chains as fallback — ARDY's
    Core-27 uses Mixamo bone names).
 3. Build RTG_ArdyToManny against the existing IK_Manny.
 4. IKRetargetBatchOperation.duplicate_and_retarget all 4 clips -> /Game/Generated/Ardy/Manny/.
"""
import os

import unreal

unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")

SRC_FBX = r"C:\Users\kazda\kiro\ardy\outputs"
ARDY_ROOT = "/Game/Generated/Ardy"
SHARED = f"{ARDY_ROOT}/Shared"
OUT = f"{ARDY_ROOT}/Manny"
IK_DIR = "/Game/Characters/Player/IK"
IK_MANNY = f"{IK_DIR}/IK_Manny"
MANNY_MESH_CANDIDATES = [
    "/MoverTests/Characters/Mannequins/Meshes/SKM_Manny",
    "/MoverExamples/Characters/Mannequins/Meshes/SKM_Manny_Simple",
]
CLIPS = ["slash", "run", "roll", "idle"]

CHAINS = {
    "Spine": ("Spine", "Spine3", ""),
    "Head": ("Neck", "Head", ""),
    "LeftArm": ("LeftShoulder", "LeftHand", "LeftHand_Goal"),
    "RightArm": ("RightShoulder", "RightHand", "RightHand_Goal"),
    "LeftLeg": ("LeftUpLeg", "LeftFoot", "LeftFoot_Goal"),
    "RightLeg": ("RightUpLeg", "RightFoot", "RightFoot_Goal"),
}

eal = unreal.EditorAssetLibrary


def step1_shared_skeleton():
    """Import slash as the rig-bearing clip under Shared, then run/roll/idle anim-only onto its skeleton."""
    tools = unreal.AssetToolsHelpers.get_asset_tools()

    def _task(fbx, dest, skeleton=None):
        t = unreal.AssetImportTask()
        t.filename = fbx
        t.destination_path = dest
        t.replace_existing = True
        t.automated = True
        t.save = True
        o = unreal.FbxImportUI()
        o.import_mesh = skeleton is None
        o.import_as_skeletal = True
        o.import_animations = True
        if skeleton is None:
            o.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
        else:
            o.skeleton = skeleton
            o.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
        t.options = o
        return t

    tools.import_asset_tasks([_task(os.path.join(SRC_FBX, "slash.fbx"), SHARED)])
    skel = eal.load_asset(f"{SHARED}/slash_Skeleton")
    if not skel:
        # skeleton asset name can vary; find it in the folder
        ar = unreal.AssetRegistryHelpers.get_asset_registry()
        for a in ar.get_assets_by_path(SHARED, recursive=True):
            if a.asset_class_path.asset_name == "Skeleton":
                skel = a.get_asset()
                break
    if not skel:
        raise RuntimeError("no shared ARDY skeleton after slash import")
    print(f"POF_RTG_SKEL {skel.get_path_name()}")

    for clip in ["run", "roll", "idle"]:
        tools.import_asset_tasks([_task(os.path.join(SRC_FBX, f"{clip}.fbx"), SHARED, skeleton=skel)])
    return skel


def step2_ardy_rig():
    path = f"{IK_DIR}/IK_ArdyCore"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)  # rebuild fresh each run (experiment-phase)
    mesh = eal.load_asset(f"{SHARED}/slash")
    if not mesh:
        raise RuntimeError("shared slash SkeletalMesh missing")
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    rig = tools.create_asset("IK_ArdyCore", IK_DIR, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    ctrl = unreal.IKRigController.get_controller(rig)
    ctrl.set_skeletal_mesh(mesh)
    matched = ctrl.apply_auto_generated_retarget_definition()
    print(f"POF_RTG_AUTOTEMPLATE {bool(matched)}")
    if not matched:
        ctrl.set_retarget_root(unreal.Name("Hips"))
        added = 0
        for name, (start, end, goal) in CHAINS.items():
            try:
                if ctrl.add_retarget_chain(name, unreal.Name(start), unreal.Name(end)):
                    added += 1
            except Exception as e:
                print(f"POF_RTG_CHAIN_FAIL {name}: {e}")
        print(f"POF_RTG_CHAINS {added}/{len(CHAINS)}")
    eal.save_asset(f"{IK_DIR}/IK_ArdyCore")
    return rig


def step3_retargeter(src_rig):
    path = f"{IK_DIR}/RTG_ArdyToManny"
    if eal.does_asset_exist(path):
        eal.delete_asset(path)
    tgt_rig = eal.load_asset(IK_MANNY)
    if not tgt_rig:
        raise RuntimeError("IK_Manny missing")
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    rtg = tools.create_asset("RTG_ArdyToManny", IK_DIR, unreal.IKRetargeter, None)
    ctrl = unreal.IKRetargeterController.get_controller(rtg)
    ctrl.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, src_rig)
    ctrl.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, tgt_rig)
    ctrl.add_default_ops()
    ctrl.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    eal.save_asset(path)
    return rtg


def step4_batch(rtg):
    src_mesh = eal.load_asset(f"{SHARED}/slash")
    tgt_mesh = None
    for p in MANNY_MESH_CANDIDATES:
        if eal.does_asset_exist(p):
            tgt_mesh = eal.load_asset(p)
            break
    if not tgt_mesh:
        raise RuntimeError("no Manny mesh found")
    print(f"POF_RTG_TARGET {tgt_mesh.get_path_name()}")

    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    anims = [a for a in ar.get_assets_by_path(SHARED, recursive=True) if a.asset_class_path.asset_name == "AnimSequence"]
    print(f"POF_RTG_ANIMS {[str(a.asset_name) for a in anims]}")

    unreal.IKRetargetBatchOperation.duplicate_and_retarget(
        anims, src_mesh, tgt_mesh, rtg, suffix="_Manny",
        include_referenced_assets=False, overwrite_existing_files=True,
    )
    # duplicate_and_retarget writes to /Game root — move results under OUT
    for a in anims:
        root_path = f"/Game/{a.asset_name}_Manny"
        dest = f"{OUT}/{a.asset_name}_Manny"
        if eal.does_asset_exist(root_path):
            if eal.does_asset_exist(dest):
                eal.delete_asset(dest)
            eal.rename_asset(root_path, dest)
        ok = eal.does_asset_exist(dest)
        length = 0.0
        if ok:
            seq = eal.load_asset(dest)
            length = float(seq.get_play_length())
            eal.save_asset(dest)
        print(f"POF_RTG_RESULT {a.asset_name}_Manny exists={ok} length={length:.2f}s")


skel = step1_shared_skeleton()
rig = step2_ardy_rig()
rtg = step3_retargeter(rig)
step4_batch(rtg)
print("POF_RTG_DONE")

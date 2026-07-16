"""ARDY v2 cycle: import roll2 + slash2 (Qwen-informed regenerations), retarget
INDIVIDUALLY (never concatenated — the concat retarget explodes), probe, build
montages, install as AM_Dodge_Forward / AM_MeleeCombo.

Caller must rm -rf Content/Generated/Ardy/V2 first (fresh-folder import rule).
"""
import os

import unreal

unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
eal = unreal.EditorAssetLibrary

SRC_FBX = r"C:\Users\kazda\kiro\ardy\outputs"
DEST = "/Game/Generated/Ardy/V2"
MANNY_OUT = "/Game/Generated/Ardy/Manny"
IK_DIR = "/Game/Characters/Player/IK"
MONTAGE_DIR = "/Game/Characters/Player/Animations/Montages"
CLIPS = ["roll2", "slash2_s7"]  # roll2 imports WITH mesh (rig bearer), slash2 anim-only

tools = unreal.AssetToolsHelpers.get_asset_tools()


def _import(clip, skeleton=None):
    t = unreal.AssetImportTask()
    t.filename = os.path.join(SRC_FBX, f"{clip}.fbx")
    t.destination_path = DEST
    t.replace_existing = False
    t.automated = True
    t.save = False
    o = unreal.FbxImportUI()
    o.import_mesh = skeleton is None
    o.import_as_skeletal = True
    o.import_animations = True
    o.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)
    o.skeletal_mesh_import_data.set_editor_property("import_morph_targets", False)
    if skeleton is None:
        o.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    else:
        o.skeleton = skeleton
        o.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    t.options = o
    tools.import_asset_tasks([t])
    print(f"POF_V2_IMPORT {clip}: {list(t.imported_object_paths)}")


_import("roll2")
src_mesh = eal.load_asset(f"{DEST}/roll2")
skel = src_mesh.get_editor_property("skeleton") if src_mesh else None
if not (src_mesh and skel):
    raise SystemExit("POF_V2_FAIL roll2 mesh/skeleton")
eal.save_asset(f"{DEST}/roll2")
eal.save_asset(skel.get_path_name().split(".")[0])
_import("slash2_s7", skeleton=skel)

# resolve + save the anims (in-memory registry filter)
ar = unreal.AssetRegistryHelpers.get_asset_registry()
flt = unreal.ARFilter(package_paths=[unreal.Name(DEST)], recursive_paths=True,
                      class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "AnimSequence")],
                      include_only_on_disk_assets=False)
found = {str(a.asset_name): a for a in ar.get_assets(flt)}
print(f"POF_V2_FOUND {sorted(found)}")
anims = []
for c in CLIPS:
    name = next((n for n in sorted(found) if n.startswith(c)), None)
    if not name:
        raise SystemExit(f"POF_V2_FAIL no anim for {c}: {sorted(found)}")
    eal.save_asset(str(found[name].package_name))
    anims.append(found[name])

# rig pair + INDIVIDUAL batch retarget (one call, separate output assets)
tgt_mesh = eal.load_asset("/MoverTests/Characters/Mannequins/Meshes/SKM_Manny")
if not tgt_mesh:
    ar.scan_paths_synchronous(["/MoverTests"], True)
    tgt_mesh = eal.load_asset("/MoverTests/Characters/Mannequins/Meshes/SKM_Manny")
tgt_rig = eal.load_asset(f"{IK_DIR}/IK_Manny")
src_rig = eal.load_asset(f"{IK_DIR}/IK_ArdyV2")
if not src_rig:
    src_rig = tools.create_asset("IK_ArdyV2", IK_DIR, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
rc = unreal.IKRigController.get_controller(src_rig)
rc.set_skeletal_mesh(src_mesh)
print(f"POF_V2_AUTOTEMPLATE {bool(rc.apply_auto_generated_retarget_definition())}")
eal.save_asset(f"{IK_DIR}/IK_ArdyV2")
rtg = eal.load_asset(f"{IK_DIR}/RTG_ArdyV2ToManny")
if not rtg:
    rtg = tools.create_asset("RTG_ArdyV2ToManny", IK_DIR, unreal.IKRetargeter, None)
    tc = unreal.IKRetargeterController.get_controller(rtg)
    tc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, src_rig)
    tc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, tgt_rig)
    tc.add_default_ops()
    tc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    eal.save_asset(f"{IK_DIR}/RTG_ArdyV2ToManny")
unreal.IKRetargetBatchOperation.duplicate_and_retarget(
    anims, src_mesh, tgt_mesh, rtg, suffix="_Manny",
    include_referenced_assets=False, overwrite_existing_files=True,
)
retargeted = {}
for a in anims:
    root_path = f"/Game/{a.asset_name}_Manny"
    dest = f"{MANNY_OUT}/{a.asset_name}_Manny"
    if eal.does_asset_exist(root_path):
        if eal.does_asset_exist(dest):
            eal.delete_asset(dest)
        eal.rename_asset(root_path, dest)
    if not eal.does_asset_exist(dest):
        raise SystemExit(f"POF_V2_FAIL retarget {dest}")
    eal.save_asset(dest)
    retargeted[str(a.asset_name)] = dest
print(f"POF_V2_RETARGETED {retargeted}")

# numeric sanity probe (anti-explosion gate) BEFORE install
opts_probe = unreal.AnimPoseEvaluationOptions()
for name, path in retargeted.items():
    seq = eal.load_asset(path)
    length = float(seq.get_play_length())
    bad = 0
    for i in range(6):
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, length * i / 5.0, opts_probe)
        for b in ["foot_l", "foot_r", "head"]:
            tr = unreal.AnimPoseExtensions.get_bone_pose(pose, unreal.Name(b), unreal.AnimPoseSpaces.WORLD)
            v = tr.translation
            if max(abs(v.x), abs(v.y), abs(v.z)) > 500.0:  # >5m from root = exploded
                bad += 1
    print(f"POF_V2_PROBE {name} len={length:.2f} badSamples={bad}")
    if bad:
        raise SystemExit(f"POF_V2_FAIL {name} retarget exploded ({bad} bad samples) — NOT installing")


def install_montage(seq_path, montage_path, backup_suffix, root_motion):
    seq = eal.load_asset(seq_path)
    if root_motion:
        seq.set_editor_property("enable_root_motion", True)
        eal.save_asset(seq_path)
    factory = unreal.AnimMontageFactory()
    factory.set_editor_property("target_skeleton", seq.get_editor_property("skeleton"))
    factory.set_editor_property("source_animation", seq)
    base = montage_path.rsplit("/", 1)[-1]
    tmp = f"{MONTAGE_DIR}/{base}_ArdyV2tmp"
    if eal.does_asset_exist(tmp):
        eal.delete_asset(tmp)
    m = tools.create_asset(f"{base}_ArdyV2tmp", MONTAGE_DIR, unreal.AnimMontage, factory)
    eal.save_asset(tmp)
    bak = f"{montage_path}_{backup_suffix}"
    if eal.does_asset_exist(bak):
        eal.delete_asset(bak)
    if eal.does_asset_exist(montage_path):
        eal.rename_asset(montage_path, bak)
    if not eal.rename_asset(tmp, montage_path):
        raise SystemExit(f"POF_V2_FAIL install {montage_path}")
    eal.save_asset(montage_path)
    print(f"POF_V2_INSTALLED {montage_path} len={float(eal.load_asset(montage_path).get_play_length()):.2f}")


install_montage(retargeted["roll2_Anim"], f"{MONTAGE_DIR}/AM_Dodge_Forward", "PreV2", root_motion=True)
install_montage(retargeted["slash2_s7_Anim"], f"{MONTAGE_DIR}/AM_MeleeCombo", "PreV2", root_motion=False)
print("POF_V2_DONE")

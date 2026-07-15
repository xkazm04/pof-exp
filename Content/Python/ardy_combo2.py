"""ARDY 3-hit melee combo v2 — single concatenated clip (comboall.fbx, sections at
0/2.5/5.0s) -> import -> retarget -> sectioned montage + notify states -> install
as the player's AM_MeleeCombo.

Import lessons baked in (see ardy-text-to-motion-spec.md):
 - FRESH destination folder + save=False + replace_existing=False, else the FBX
   importer takes a reimport path that silently SKIPS AnimSequence creation
 - stale .uasset files survive EditorAssetLibrary.delete_directory — rm the folder
   on the filesystem BEFORE launching UE
 - explicit save_asset for skeleton + anim (task.save covers the mesh only)
 - /MoverTests plugin content needs scan_paths_synchronous before load_asset
"""
import os

import unreal

unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
eal = unreal.EditorAssetLibrary

SRC_FBX = r"C:\Users\kazda\kiro\ardy\outputs\comboall.fbx"
DEST = "/Game/Generated/Ardy/ComboAll"
MANNY = "/Game/Generated/Ardy/Manny"
IK_DIR = "/Game/Characters/Player/IK"
MONTAGE_DIR = "/Game/Characters/Player/Animations/Montages"
OLD = f"{MONTAGE_DIR}/AM_MeleeCombo"
BAK = f"{MONTAGE_DIR}/AM_MeleeCombo_PreArdy"
SECTION_STARTS = [0.0, 2.5, 5.0]

# ---- import (fresh folder is prepared by the caller via rm -rf)
tools = unreal.AssetToolsHelpers.get_asset_tools()
t = unreal.AssetImportTask()
t.filename = SRC_FBX
t.destination_path = DEST
t.replace_existing = False
t.automated = True
t.save = False
o = unreal.FbxImportUI()
o.import_mesh = True
o.import_as_skeletal = True
o.import_animations = True
o.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
o.anim_sequence_import_data.set_editor_property("import_bone_tracks", True)
o.skeletal_mesh_import_data.set_editor_property("import_morph_targets", False)
t.options = o
tools.import_asset_tasks([t])
src_mesh = eal.load_asset(f"{DEST}/comboall")
anim = eal.load_asset(f"{DEST}/comboall_Anim")
if not (src_mesh and anim):
    raise SystemExit(f"POF_COMBO2_FAIL import mesh={bool(src_mesh)} anim={bool(anim)}")
skel = src_mesh.get_editor_property("skeleton")
for ap in (f"{DEST}/comboall", skel.get_path_name().split(".")[0], f"{DEST}/comboall_Anim"):
    eal.save_asset(ap)
print(f"POF_COMBO2_IMPORTED anim len={float(anim.get_play_length()):.2f}")

# ---- retarget (fresh rig pair, auto-template)
ar = unreal.AssetRegistryHelpers.get_asset_registry()
tgt_mesh = eal.load_asset("/MoverTests/Characters/Mannequins/Meshes/SKM_Manny")
if not tgt_mesh:
    ar.scan_paths_synchronous(["/MoverTests"], True)
    tgt_mesh = eal.load_asset("/MoverTests/Characters/Mannequins/Meshes/SKM_Manny")
tgt_rig = eal.load_asset(f"{IK_DIR}/IK_Manny")
src_rig = eal.load_asset(f"{IK_DIR}/IK_ArdyComboAll")
if not src_rig:
    src_rig = tools.create_asset("IK_ArdyComboAll", IK_DIR, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
rc = unreal.IKRigController.get_controller(src_rig)
rc.set_skeletal_mesh(src_mesh)
print(f"POF_COMBO2_AUTOTEMPLATE {bool(rc.apply_auto_generated_retarget_definition())}")
eal.save_asset(f"{IK_DIR}/IK_ArdyComboAll")
rtg = eal.load_asset(f"{IK_DIR}/RTG_ArdyComboAllToManny")
if not rtg:
    rtg = tools.create_asset("RTG_ArdyComboAllToManny", IK_DIR, unreal.IKRetargeter, None)
    tc = unreal.IKRetargeterController.get_controller(rtg)
    tc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, src_rig)
    tc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, tgt_rig)
    tc.add_default_ops()
    tc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
    eal.save_asset(f"{IK_DIR}/RTG_ArdyComboAllToManny")
ad = unreal.AssetRegistryHelpers.create_asset_data(anim)
unreal.IKRetargetBatchOperation.duplicate_and_retarget(
    [ad], src_mesh, tgt_mesh, rtg, suffix="_Manny",
    include_referenced_assets=False, overwrite_existing_files=True,
)
root_path = "/Game/comboall_Anim_Manny"
dest_anim = f"{MANNY}/comboall_Anim_Manny"
if eal.does_asset_exist(root_path):
    if eal.does_asset_exist(dest_anim):
        eal.delete_asset(dest_anim)
    eal.rename_asset(root_path, dest_anim)
if not eal.does_asset_exist(dest_anim):
    raise SystemExit("POF_COMBO2_FAIL retarget output missing")
eal.save_asset(dest_anim)
print(f"POF_COMBO2_RETARGETED {dest_anim}")

# ---- montage: single source (whole 7.5s clip), sections at the known concat offsets
seq = eal.load_asset(dest_anim)
factory = unreal.AnimMontageFactory()
factory.set_editor_property("target_skeleton", seq.get_editor_property("skeleton"))
factory.set_editor_property("source_animation", seq)
tmp_name = "AM_ArdyCombo"
tmp_path = f"{MONTAGE_DIR}/{tmp_name}"
if eal.does_asset_exist(tmp_path):
    eal.delete_asset(tmp_path)
m = tools.create_asset(tmp_name, MONTAGE_DIR, unreal.AnimMontage, factory)
if not m:
    raise SystemExit("POF_COMBO2_FAIL montage create")

sections = []
for i, s in enumerate(SECTION_STARTS):
    try:
        sec = unreal.CompositeSection(section_name=unreal.Name(f"Combo{i + 1}"), start_time=s)
    except Exception:
        sec = unreal.CompositeSection()
        try:
            sec.set_editor_property("section_name", unreal.Name(f"Combo{i + 1}"))
            sec.set_editor_property("start_time", s)
        except Exception as e:
            print(f"POF_COMBO2_SECTION_FAIL {i}: {e}")
            sec = None
    if sec:
        sections.append(sec)
try:
    m.set_editor_property("composite_sections", sections)
    print(f"POF_COMBO2_SECTIONS {[str(x.get_editor_property('section_name')) for x in m.get_editor_property('composite_sections')]}")
except Exception as e:
    print(f"POF_COMBO2_SECTIONS_FAIL {e} — montage keeps the factory default single section")

# notify states (game module loads in this project's commandlet)
combo_cls = unreal.load_class(None, "/Script/PoF.AnimNotifyState_ComboWindow")
hit_cls = unreal.load_class(None, "/Script/PoF.AnimNotifyState_HitDetection")
print(f"POF_COMBO2_NOTIFY_CLASSES combo={bool(combo_cls)} hit={bool(hit_cls)}")
notifies = []
if combo_cls and hit_cls:
    seg_len = 2.5
    for s in SECTION_STARTS:
        for cls, f0, fl in ((hit_cls, 0.25, 0.30), (combo_cls, 0.55, 0.40)):
            try:
                ev = unreal.AnimNotifyEvent()
                state = unreal.new_object(cls, outer=m)
                ev.set_editor_property("notify_state_class", state)
                ev.set_editor_property("time", s + seg_len * f0)
                ev.set_editor_property("duration", seg_len * fl)
                notifies.append(ev)
            except Exception as e:
                print(f"POF_COMBO2_NOTIFY_FAIL {e}")
    try:
        m.set_editor_property("notifies", notifies)
    except Exception as e:
        print(f"POF_COMBO2_NOTIFIES_SET_FAIL {e}")
print(f"POF_COMBO2_NOTIFIES {len(notifies)}")
eal.save_asset(tmp_path)
print(f"POF_COMBO2_MONTAGE len={float(m.get_play_length()):.2f}")

# ---- install
old_m = eal.load_asset(OLD)
if old_m:
    if eal.does_asset_exist(BAK):
        eal.delete_asset(BAK)
    if not eal.rename_asset(OLD, BAK):
        raise SystemExit("POF_COMBO2_FAIL backup rename")
if not eal.rename_asset(tmp_path, OLD):
    if old_m:
        eal.rename_asset(BAK, OLD)
    raise SystemExit("POF_COMBO2_FAIL install rename (original restored)")
eal.save_asset(OLD)
print(f"POF_COMBO2_INSTALLED {OLD} (backup: {BAK})")
print("POF_COMBO2_DONE")

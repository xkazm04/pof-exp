"""Install the ARDY roll as the player's forward-dodge montage.

Findings that shape this script (asset-registry probes):
 - AM_Dodge_Forward is an EMPTY placeholder montage (no skeleton, no segments, no deps).
 - BP_VSPlayer depends on /MoverTests/SKM_Manny — the same skeleton family as
   roll_Anim_Manny (SK_Mannequin), so the retargeted clip is directly playable.

Headless: UnrealEditor-Cmd <uproject> -run=pythonscript -script=... -nullrhi
"""
import unreal

eal = unreal.EditorAssetLibrary
MONTAGE_DIR = "/Game/Characters/Player/Animations/Montages"
OLD = f"{MONTAGE_DIR}/AM_Dodge_Forward"
BAK = f"{MONTAGE_DIR}/AM_Dodge_Forward_PreArdy"
ROLL = "/Game/Generated/Ardy/Manny/roll_Anim_Manny"

roll = eal.load_asset(ROLL)
if not roll:
    raise SystemExit("POF_MONTAGE_FAIL roll missing")
roll_skel = roll.get_editor_property("skeleton")
print(f"POF_MONTAGE_SKEL roll={roll_skel.get_path_name() if roll_skel else None}")

# root motion: GA_Dodge drives displacement from montage root motion
print(f"POF_MONTAGE_ROLL_RM_BEFORE {roll.get_editor_property('enable_root_motion')}")
roll.set_editor_property("enable_root_motion", True)
eal.save_asset(ROLL)
print("POF_MONTAGE_ROLL_RM_SET True")

old_m = eal.load_asset(OLD)
old_slots = [str(t.slot_name) for t in old_m.get_editor_property("slot_anim_tracks")] if old_m else []
print(f"POF_MONTAGE_OLD slots={old_slots} len={float(old_m.get_play_length()):.2f}" if old_m else "POF_MONTAGE_OLD missing")

tools = unreal.AssetToolsHelpers.get_asset_tools()
factory = unreal.AnimMontageFactory()
factory.set_editor_property("target_skeleton", roll_skel)
factory.set_editor_property("source_animation", roll)
tmp_name = "AM_ArdyRoll_Fwd"
tmp_path = f"{MONTAGE_DIR}/{tmp_name}"
if eal.does_asset_exist(tmp_path):
    eal.delete_asset(tmp_path)
new_m = tools.create_asset(tmp_name, MONTAGE_DIR, unreal.AnimMontage, factory)
if not new_m:
    raise SystemExit("POF_MONTAGE_FAIL create_asset")

new_slots = [str(t.slot_name) for t in new_m.get_editor_property("slot_anim_tracks")]
# keep the original slot name if the placeholder had one; otherwise the factory default stands
if old_slots and new_slots and old_slots[0] != new_slots[0]:
    tracks = new_m.get_editor_property("slot_anim_tracks")
    tracks[0].set_editor_property("slot_name", unreal.Name(old_slots[0]))
    new_m.set_editor_property("slot_anim_tracks", tracks)
eal.save_asset(tmp_path)
print(f"POF_MONTAGE_NEW len={float(new_m.get_play_length()):.2f} slots={[str(t.slot_name) for t in new_m.get_editor_property('slot_anim_tracks')]}")

if old_m:
    if eal.does_asset_exist(BAK):
        eal.delete_asset(BAK)
    if not eal.rename_asset(OLD, BAK):
        raise SystemExit("POF_MONTAGE_FAIL backup rename")
if not eal.rename_asset(tmp_path, OLD):
    if old_m:
        eal.rename_asset(BAK, OLD)
    raise SystemExit("POF_MONTAGE_FAIL install rename (original restored)")
eal.save_asset(OLD)
print(f"POF_MONTAGE_INSTALLED {OLD} (backup: {BAK})")
print("POF_MONTAGE_DONE")

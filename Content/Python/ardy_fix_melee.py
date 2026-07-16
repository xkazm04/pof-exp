"""Replace the broken concatenated melee montage with a single clean slash
(combo1_Anim_Manny — numerically verified: feet ~6cm, head ~144cm).
The concat clip's retarget exploded (bones at 78m); singles are clean."""
import unreal

eal = unreal.EditorAssetLibrary
MONTAGE_DIR = "/Game/Characters/Player/Animations/Montages"
OLD = f"{MONTAGE_DIR}/AM_MeleeCombo"
BROKEN_BAK = f"{MONTAGE_DIR}/AM_MeleeCombo_ArdyConcatBroken"
SEQ = "/Game/Generated/Ardy/Manny/combo1_Anim_Manny"

seq = eal.load_asset(SEQ)
if not seq:
    raise SystemExit("POF_FIX_FAIL combo1_Anim_Manny missing")

tools = unreal.AssetToolsHelpers.get_asset_tools()
factory = unreal.AnimMontageFactory()
factory.set_editor_property("target_skeleton", seq.get_editor_property("skeleton"))
factory.set_editor_property("source_animation", seq)
tmp = f"{MONTAGE_DIR}/AM_ArdySlash1"
if eal.does_asset_exist(tmp):
    eal.delete_asset(tmp)
m = tools.create_asset("AM_ArdySlash1", MONTAGE_DIR, unreal.AnimMontage, factory)
if not m:
    raise SystemExit("POF_FIX_FAIL montage create")
eal.save_asset(tmp)

if eal.does_asset_exist(BROKEN_BAK):
    eal.delete_asset(BROKEN_BAK)
if eal.does_asset_exist(OLD):
    eal.rename_asset(OLD, BROKEN_BAK)
if not eal.rename_asset(tmp, OLD):
    raise SystemExit("POF_FIX_FAIL install")
eal.save_asset(OLD)
print(f"POF_FIX_INSTALLED {OLD} len={float(eal.load_asset(OLD).get_play_length()):.2f} (broken concat kept at {BROKEN_BAK})")
print("POF_FIX_DONE")

"""The melee ability BP references /Game/Weapons/AM_SwordSlash (NOT AM_MeleeCombo).
Install the ARDY slash2 montage at THAT path (backup kept)."""
import unreal
eal = unreal.EditorAssetLibrary
SEQ = "/Game/Generated/Ardy/Manny/slash2_s7_Anim_Manny"
OLD = "/Game/Weapons/AM_SwordSlash"
BAK = "/Game/Weapons/AM_SwordSlash_PreArdy"
seq = eal.load_asset(SEQ)
tools = unreal.AssetToolsHelpers.get_asset_tools()
f = unreal.AnimMontageFactory()
f.set_editor_property("target_skeleton", seq.get_editor_property("skeleton"))
f.set_editor_property("source_animation", seq)
tmp = "/Game/Weapons/AM_ArdySlash2"
if eal.does_asset_exist(tmp):
    eal.delete_asset(tmp)
m = tools.create_asset("AM_ArdySlash2", "/Game/Weapons", unreal.AnimMontage, f)
eal.save_asset(tmp)
if eal.does_asset_exist(BAK):
    eal.delete_asset(BAK)
if not eal.rename_asset(OLD, BAK):
    raise SystemExit("POF_FIX2_FAIL backup")
if not eal.rename_asset(tmp, OLD):
    eal.rename_asset(BAK, OLD)
    raise SystemExit("POF_FIX2_FAIL install")
eal.save_asset(OLD)
print(f"POF_FIX2_INSTALLED {OLD} len={float(eal.load_asset(OLD).get_play_length()):.2f} (backup {BAK})")

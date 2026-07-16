"""Point BP_GA_MeleeAttack.AttackMontage directly at the ARDY slash montage (CDO edit),
and fix up the redirector debris under /Game/Weapons left by the rename installs."""
import unreal
eal = unreal.EditorAssetLibrary

# clean redirectors so path resolution is sane again
fixed = []
ar = unreal.AssetRegistryHelpers.get_asset_registry()
for a in ar.get_assets_by_path("/Game/Weapons", recursive=True):
    if a.asset_class_path.asset_name == "ObjectRedirector":
        fixed.append(str(a.package_name))
if fixed:
    unreal.AssetToolsHelpers.get_asset_tools().fixup_referencers([unreal.load_asset(p) for p in fixed])
print(f"POF_FIX3_REDIRECTORS {fixed}")

montage = eal.load_asset("/Game/Weapons/AM_SwordSlash")
print(f"POF_FIX3_MONTAGE {montage.get_path_name() if montage else None} len={float(montage.get_play_length()) if montage else 0:.2f}")

bp = eal.load_asset("/Game/Abilities/BP_GA_MeleeAttack")
gc = bp.generated_class()
cdo = unreal.get_default_object(gc)
before = cdo.get_editor_property("attack_montage")
cdo.set_editor_property("attack_montage", montage)
eal.save_asset("/Game/Abilities/BP_GA_MeleeAttack")
# verify by re-reading
cdo2 = unreal.get_default_object(bp.generated_class())
after = cdo2.get_editor_property("attack_montage")
print(f"POF_FIX3_CDO before={before.get_name() if before else None} after={after.get_name() if after else None}")
print("POF_FIX3_DONE")

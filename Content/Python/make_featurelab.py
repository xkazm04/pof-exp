"""Create the clean FeatureLab map (headless commandlet - engine classes only).

    UnrealEditor-Cmd PoF.uproject -run=pythonscript -script=Content/Python/make_featurelab.py -nullrhi

Loads the engine default template (floor + light + sky) and saves it as
/Game/Maps/FeatureLab, guaranteeing a PlayerStart. The map itself stays CLEAN -
features are spawned at runtime by UPoFFeatureLabSubsystem (code-as-data
roster), never baked into the level.
"""
import unreal

DST = "/Game/Maps/FeatureLab"
TEMPLATE = "/Engine/Maps/Templates/Template_Default"

eal = unreal.EditorAssetLibrary

# Remove a previous (possibly void) FeatureLab so the template save-as is clean.
if eal.does_asset_exist(DST):
    unreal.log(f"[featurelab] removing existing {DST}")
    eal.delete_asset(DST)

world = unreal.EditorLoadingAndSavingUtils.load_map(TEMPLATE)
name = world.get_name() if world else None
unreal.log(f"[featurelab] template loaded: {name}")
if not world or name == "Untitled":
    raise RuntimeError(f"template did not load: {TEMPLATE}")

actors = unreal.EditorLevelLibrary.get_all_level_actors()
unreal.log(f"[featurelab] template actors: {len(actors)}")
if not any(isinstance(a, unreal.PlayerStart) for a in actors):
    start = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PlayerStart, unreal.Vector(0.0, 0.0, 120.0))
    unreal.log(f"[featurelab] spawned PlayerStart: {start is not None}")
else:
    unreal.log("[featurelab] PlayerStart already present")

saved = unreal.EditorLoadingAndSavingUtils.save_map(world, DST)
unreal.log(f"[featurelab] RESULT save={bool(saved)}")

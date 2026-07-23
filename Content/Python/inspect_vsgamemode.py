"""Read BP_VSGameMode's controller/pawn/HUD classes + FeatureLab's world settings
game-mode override (the F-binding suspect list, memory featurelab-interact-binding).

    UnrealEditor-Cmd PoF.uproject -run=pythonscript -script=Content/Python/inspect_vsgamemode.py -nullrhi
"""
import unreal

gm = unreal.EditorAssetLibrary.load_blueprint_class('/Game/VerticalSlice/BP_VSGameMode')
if gm:
    cdo = unreal.get_default_object(gm)
    for prop in ('player_controller_class', 'default_pawn_class', 'hud_class'):
        try:
            unreal.log(f"[gm-inspect] BP_VSGameMode.{prop} = {cdo.get_editor_property(prop)}")
        except Exception as e:
            unreal.log(f"[gm-inspect] BP_VSGameMode.{prop} ERROR {e}")
else:
    unreal.log("[gm-inspect] BP_VSGameMode NOT FOUND")

# FeatureLab per-map override
w = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Maps/FeatureLab')
ws = unreal.EditorLevelLibrary.get_editor_world().get_world_settings() if hasattr(unreal, 'EditorLevelLibrary') else None
try:
    sub = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = sub.get_editor_world()
    settings = world.get_world_settings()
    unreal.log(f"[gm-inspect] FeatureLab WorldSettings override = {settings.get_editor_property('default_game_mode')}")
except Exception as e:
    unreal.log(f"[gm-inspect] FeatureLab override ERROR {e}")

unreal.log("[gm-inspect] RESULT done")

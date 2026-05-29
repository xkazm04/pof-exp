"""Create /Game/Maps/TestHarness: an open lit floor with the player's game mode, for
clean sustained-movement / roll scenarios (the arena walls cramp travel)."""
import unreal
def run(args):
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.new_level("/Game/Maps/TestHarness")
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    out = {"spawned": []}
    plane = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Plane")
    floor = eas.spawn_actor_from_object(plane, unreal.Vector(0,0,0)) if plane else None
    if floor:
        floor.set_actor_scale3d(unreal.Vector(400,400,1)); out["spawned"].append("floor")
    dl = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0,0,800), unreal.Rotator(-50,30,0))
    if dl:
        c = dl.get_component_by_class(unreal.DirectionalLightComponent)
        if c: c.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE); c.set_intensity(6.0)
        out["spawned"].append("dirlight")
    sk = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0,0,800))
    if sk:
        sc = sk.get_component_by_class(unreal.SkyLightComponent)
        if sc: sc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE); sc.set_intensity(3.0); sc.recapture_sky()
        out["spawned"].append("skylight")
    if eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0,0,120)): out["spawned"].append("playerstart")
    gm = unreal.EditorAssetLibrary.load_asset("/Game/VerticalSlice/BP_VSGameMode")
    for a in eas.get_all_level_actors():
        if isinstance(a, unreal.WorldSettings) and gm:
            a.set_editor_property("default_game_mode", gm.generated_class()); out["gamemode_set"]=True
    les.save_current_level()
    return out

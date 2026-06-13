"""Create /Game/Maps/TestHarness: an open, brightly-lit floor with the player's game mode,
for clean sustained-movement / roll scenarios (the arena walls cramp travel + spawn an enemy).

Lighting note: the previous version was too dark for capture because its SkyLight had no sky
to capture (no atmosphere) -> near-black FinalColor. This builds a real dynamic daylight:
SkyAtmosphere + a sun directional light + a real-time-capture SkyLight, so both the viewport
and the SceneCapture render bright without baking.
"""
import unreal


def run(args):
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.new_level("/Game/Maps/TestHarness")
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    out = {"spawned": []}

    plane = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Plane")
    floor = eas.spawn_actor_from_object(plane, unreal.Vector(0, 0, 0)) if plane else None
    if floor:
        floor.set_actor_scale3d(unreal.Vector(400, 400, 1))
        out["spawned"].append("floor")
        # Unlit emissive floor so SceneCapture shows the floor line (it crushed to black before),
        # letting the skin-vs-floor clip be seen.
        fmat = unreal.EditorAssetLibrary.load_asset("/Game/Maps/M_FloorRef")
        comp = floor.get_component_by_class(unreal.StaticMeshComponent) if floor else None
        if fmat and comp:
            comp.set_material(0, fmat)
            out["floor_material"] = True

    # Sun — drives the SkyAtmosphere and lights the scene.
    dl = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 800), unreal.Rotator(-46, 30, 0))
    if dl:
        c = dl.get_component_by_class(unreal.DirectionalLightComponent)
        if c:
            c.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
            c.set_intensity(10.0)
            try:
                c.set_editor_property("atmosphere_sun_light", True)
            except Exception as e:
                out["sun_atmo_err"] = str(e)
        out["spawned"].append("sun")

    # Sky atmosphere — gives the SkyLight something to capture + sky ambient.
    try:
        if eas.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0)):
            out["spawned"].append("skyatmosphere")
    except Exception as e:
        out["skyatmosphere_err"] = str(e)

    # SkyLight — ambient fill; real-time capture pulls light from the atmosphere (no bake).
    sk = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 300))
    if sk:
        sc = sk.get_component_by_class(unreal.SkyLightComponent)
        if sc:
            sc.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
            try:
                sc.set_editor_property("real_time_capture", True)
            except Exception as e:
                out["skylight_rtc_err"] = str(e)
            sc.set_intensity(1.0)
            sc.recapture_sky()
        out["spawned"].append("skylight")

    if eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 120)):
        out["spawned"].append("playerstart")

    gm = unreal.EditorAssetLibrary.load_asset("/Game/VerticalSlice/BP_VSGameMode")
    for a in eas.get_all_level_actors():
        if isinstance(a, unreal.WorldSettings) and gm:
            a.set_editor_property("default_game_mode", gm.generated_class())
            out["gamemode_set"] = True

    les.save_current_level()
    return out

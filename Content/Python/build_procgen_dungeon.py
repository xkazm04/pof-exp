"""
build_procgen_dungeon.py
========================
Authors 3 RoomTemplate data assets + a new /Game/Maps/ProcGenDungeon level with a
seeded ARPGLevelGenerator, then calls GenerateLevel() to BAKE a walkable
multi-room greybox dungeon into the saved map (edit-time generation).

Run via the FULL editor (level Python needs Slate):
    UnrealEditor.exe <uproject> -ExecutePythonScript="<abs path>" -unattended -nopause -nosplash
"""
import unreal

ROOM_TEMPLATES_DIR = "/Game/Level/RoomTemplates"
LEVEL_PATH = "/Game/Maps/ProcGenDungeon"
TARGET_ROOMS = 6
SEED = 1337
ROOM = 800.0

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_lib = unreal.EditorAssetLibrary
D = unreal.RoomConnectionDirection


def _log(m):
    unreal.log("[build_procgen] " + m)


def _offset(direction, half):
    if direction == D.NORTH:
        return unreal.Vector(half, 0.0, 0.0)
    if direction == D.SOUTH:
        return unreal.Vector(-half, 0.0, 0.0)
    if direction == D.EAST:
        return unreal.Vector(0.0, half, 0.0)
    return unreal.Vector(0.0, -half, 0.0)  # WEST


def _slot(direction):
    s = unreal.RoomConnectionSlot()
    s.set_editor_property("direction", direction)
    s.set_editor_property("connection_width", 300.0)
    s.set_editor_property("local_offset", _offset(direction, ROOM * 0.5))
    return s


def make_template(name, directions):
    path = "%s/%s" % (ROOM_TEMPLATES_DIR, name)
    # Idempotent: reconfigure the existing asset on a re-run (create_asset
    # returns None if the path is occupied), else create a fresh one.
    if asset_lib.does_asset_exist(path):
        tmpl = asset_lib.load_asset(path)
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.ARPGRoomTemplate)
        tmpl = asset_tools.create_asset(name, ROOM_TEMPLATES_DIR, unreal.ARPGRoomTemplate, factory)
    if tmpl is None:
        raise RuntimeError("failed to create/load template " + path)
    tmpl.set_editor_property("template_id", unreal.Name(name))
    tmpl.set_editor_property("room_size", unreal.Vector(ROOM, ROOM, 300.0))
    tmpl.set_editor_property("connection_slots", [_slot(d) for d in directions])
    tmpl.set_editor_property("selection_weight", 1.0)
    asset_lib.save_asset(path)
    n = len(tmpl.get_editor_property("connection_slots"))
    _log("Template %s: %d slots (expected %d)" % (name, n, len(directions)))
    return tmpl


def main():
    _log("=== ProcGen dungeon build START ===")
    rt_hub = make_template("RT_Hub", [D.NORTH, D.SOUTH, D.EAST, D.WEST])
    rt_room = make_template("RT_Room", [D.NORTH, D.SOUTH])
    rt_end = make_template("RT_End", [D.SOUTH])

    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.new_level(LEVEL_PATH)
    _log("Created level: " + LEVEL_PATH)

    aes = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    dl = aes.spawn_actor_from_class(unreal.DirectionalLight,
                                    unreal.Vector(0.0, 0.0, 1000.0),
                                    unreal.Rotator(0.0, -50.0, -35.0))
    for c in dl.get_components_by_class(unreal.DirectionalLightComponent):
        c.set_mobility(unreal.ComponentMobility.MOVABLE)
        c.set_editor_property("intensity", 6.0)

    sl = aes.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0.0, 0.0, 1000.0))
    for c in sl.get_components_by_class(unreal.SkyLightComponent):
        c.set_mobility(unreal.ComponentMobility.MOVABLE)
        c.set_editor_property("intensity", 3.0)
        try:
            c.set_editor_property("real_time_capture", True)
        except Exception:
            pass

    # Player spawns here via the project GlobalDefaultGameMode (BP_VSGameMode).
    aes.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0.0, 0.0, 120.0))

    gen = aes.spawn_actor_from_class(unreal.ARPGLevelGenerator, unreal.Vector(0.0, 0.0, 0.0))
    gen.set_editor_property("room_template_pool", [rt_room])
    gen.set_editor_property("start_room_template", rt_hub)
    gen.set_editor_property("end_room_template", rt_end)
    gen.set_editor_property("target_room_count", TARGET_ROOMS)
    gen.set_editor_property("random_seed", SEED)
    gen.set_editor_property("room_padding", 0.0)
    gen.set_editor_property("min_corridor_length", 2.0)
    gen.set_editor_property("max_corridor_length", 2.0)
    gen.set_editor_property("spawn_blockout_actors", True)

    # Edit-time bake: spawns AARPGBlockoutRoom actors into the level.
    gen.generate_level()

    rooms = [a for a in aes.get_all_level_actors()
             if isinstance(a, unreal.ARPGBlockoutRoom)]
    _log("Baked %d BlockoutRoom actors (target %d)" % (len(rooms), TARGET_ROOMS))
    if len(rooms) != TARGET_ROOMS:
        unreal.log_warning("[build_procgen] room count %d != target %d "
                           "(seed/connectivity may need tuning)" % (len(rooms), TARGET_ROOMS))

    les.save_current_level()
    _log("Saved level: " + LEVEL_PATH)
    _log("=== ProcGen dungeon build COMPLETE ===")


if __name__ == "__main__":
    main()
    try:
        if unreal.is_editor():
            unreal.SystemLibrary.quit_editor()
    except Exception:
        pass

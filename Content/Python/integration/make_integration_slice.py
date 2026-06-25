"""
make_integration_slice.py — assemble the vertical-slice integration map.

Composes three streams into one map /Game/Maps/Integration_Slice:
  - Character: BP_JediPlayer spawns (via BP_JediGameMode) — the robed Jedi + saber.
  - Inventory: an AARPGLootChest (LT_ChestPotion) placed in front of the spawn.
  - Environment stand-in: the lit VerticalSlice arena (Arena_Ancient is still the
    empty Phase-0 skeleton on the trunk; swap to it once Environment merges).
  - plus hero lights so the Jedi reads (the locked-exposure recipe from Stream 2).

Commandlet-safe: duplicate_asset + EditorLoadingAndSavingUtils.load_map/save_map
(new_level crashes headless). Writes Saved/make_integration_slice.json.
"""
import json
import math
import unreal

EAL = unreal.EditorAssetLibrary
SRC = "/Game/Maps/Arena_Ancient"   # Environment stream's sandy arena (was VerticalSlice stand-in)
DST = "/Game/Maps/Integration_Slice"
GM = "/Game/Characters/Jedi/BP_JediGameMode"
LOOT_TABLE = "/Game/Inventory/LT_ChestPotion"
CHEST_CLASS = "/Script/PoF.ARPGLootChest"
CUBE = "/Engine/BasicShapes/Cube"
CYL = "/Engine/BasicShapes/Cylinder"
CHEST_FWD = 250.0
OUT = {"steps": []}


def _log(m):
    unreal.log_warning("[integration] " + str(m))
    OUT["steps"].append(str(m))


def add_spot(eas, label, loc, target, intensity, color, cone, radius):
    sl = eas.spawn_actor_from_class(unreal.SpotLight, loc)
    sl.set_actor_label(label)
    sl.set_actor_rotation(unreal.MathLibrary.find_look_at_rotation(loc, target), False)
    c = sl.get_component_by_class(unreal.SpotLightComponent)
    c.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    c.set_intensity(intensity)
    c.set_light_color(color)
    c.set_editor_property("outer_cone_angle", cone)
    c.set_editor_property("inner_cone_angle", max(0.0, cone - 12.0))
    c.set_editor_property("attenuation_radius", radius)


def main():
    _log("=== MAKE INTEGRATION SLICE START ===")
    if EAL.does_asset_exist(DST):
        EAL.delete_asset(DST)
    if EAL.does_asset_exist(DST + "_BuiltData"):
        EAL.delete_asset(DST + "_BuiltData")
    if not EAL.duplicate_asset(SRC, DST):
        _log("[gate] RESULT=FAIL (duplicate)")
        return
    if EAL.does_asset_exist(SRC + "_BuiltData"):
        EAL.duplicate_asset(SRC + "_BuiltData", DST + "_BuiltData")
    _log("duplicated %s -> %s" % (SRC, DST))

    world = unreal.EditorLoadingAndSavingUtils.load_map(DST)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # Game mode -> Jedi
    gm_bp = EAL.load_asset(GM)
    for ws in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.WorldSettings):
        ws.set_editor_property("default_game_mode", gm_bp.generated_class())
    _log("game mode -> BP_JediGameMode")

    # Remove enemies + find PlayerStart
    spawn, ps_rot = unreal.Vector(0, 0, 120), unreal.Rotator(0, 0, 0)
    removed = 0
    for a in list(eas.get_all_level_actors()):
        cn = a.get_class().get_name()
        if "Enemy" in cn:
            eas.destroy_actor(a); removed += 1
        elif isinstance(a, unreal.PlayerStart):
            spawn = a.get_actor_location(); ps_rot = a.get_actor_rotation()
    _log("removed %d enemy(ies); spawn (%.0f,%.0f,%.0f) yaw=%.0f" % (removed, spawn.x, spawn.y, spawn.z, ps_rot.yaw))

    # Chest in front of the player
    yaw = math.radians(ps_rot.yaw)
    fwd = unreal.Vector(math.cos(yaw), math.sin(yaw), 0.0)
    chest_loc = unreal.Vector(spawn.x + fwd.x * CHEST_FWD, spawn.y + fwd.y * CHEST_FWD, spawn.z - 30)
    chest_cls = unreal.load_class(None, CHEST_CLASS)
    chest = eas.spawn_actor_from_class(chest_cls, chest_loc, unreal.Rotator(0, 0, ps_rot.yaw + 180.0))
    chest.set_actor_label("PotionChest")
    chest.set_editor_property("loot_table", EAL.load_asset(LOOT_TABLE))
    chest.set_editor_property("num_rolls", 1)
    chest.set_editor_property("item_scatter_radius", 0.0)
    cube = EAL.load_asset(CUBE)
    for prop, scale, zoff in (("base_mesh", (1.0, 1.2, 0.6), 0.0), ("lid_mesh", (1.0, 1.2, 0.2), 40.0)):
        try:
            mc = chest.get_editor_property(prop)
            if mc:
                mc.set_editor_property("static_mesh", cube)
                mc.set_relative_scale3d(unreal.Vector(*scale))
                if zoff:
                    mc.set_relative_location(unreal.Vector(0, 0, zoff), False, False)
        except Exception as e:
            _log("chest mesh %s warn: %s" % (prop, e))
    _log("placed PotionChest at (%.0f,%.0f,%.0f) loot=LT_ChestPotion" % (chest_loc.x, chest_loc.y, chest_loc.z))

    # Lighting: PURE native VerticalSlice arena lighting — it rendered the Jedi as a
    # clean moody shot in the Stream-2 captures at this exact spawn. Any added spot/PPV
    # over-brightened (bright floor pool + saber bloom column). Add nothing.
    _log("native VerticalSlice lighting (no added lights)")

    saved = unreal.EditorLoadingAndSavingUtils.save_map(world, DST)
    _log("save_map ok=%s" % saved)
    OUT["result"] = "PASS" if saved else "FAIL"
    out_path = unreal.Paths.combine([unreal.Paths.project_saved_dir(), "make_integration_slice.json"])
    with open(out_path, "w", encoding="utf-8") as fh:
        json.dump(OUT, fh, indent=2)
    _log("[gate] RESULT=%s" % OUT["result"])


main()

"""Live-set player movement feel knobs on the BP_VSPlayer CDO (no recompile).

The values are EditAnywhere UPROPERTYs read every tick by UpdateCursorAim /
UpdateAcceleration / UpdateRotationRate, so a CDO change applies on the next PIE
spawn (stop+Play). Call via /pof/python/run
{module:"player_movement.tune_movement", function:"run", args:{<snake_case_prop>: value, ...}}.

Recognized props (snake_case of the C++ members):
  cursor_aim_rotation_speed                       (AARPGPlayerCharacter)
  walk_speed sprint_speed acceleration_from_idle acceleration_at_full_speed
  braking_deceleration movement_ground_friction rotation_rate_idle
  rotation_rate_at_speed dodge_montage_play_rate  (AARPGCharacterBase)
"""
import unreal

BP = "/Game/VerticalSlice/BP_VSPlayer.BP_VSPlayer_C"
KNOWN = {
    "cursor_aim_rotation_speed", "walk_speed", "sprint_speed",
    "acceleration_from_idle", "acceleration_at_full_speed", "braking_deceleration",
    "movement_ground_friction", "rotation_rate_idle", "rotation_rate_at_speed",
    "dodge_montage_play_rate", "dodge_distance", "dodge_duration", "dodge_max_mesh_lift",
    "dodge_mesh_lift", "dodge_mesh_lift_speed",
}


def run(args):
    cls = unreal.load_object(None, BP)
    cdo = unreal.get_default_object(cls)
    applied, skipped = {}, {}
    for k, v in args.items():
        if k not in KNOWN:
            continue
        try:
            cdo.set_editor_property(k, float(v))
            applied[k] = float(v)
        except Exception as e:  # surface the reason, don't fail silently
            skipped[k] = str(e)
    # Persist so the value survives editor restarts (best-effort — the in-memory CDO
    # change already applies to the next PIE spawn even if the save can't run).
    saved = False
    try:
        bp = unreal.load_asset("/Game/VerticalSlice/BP_VSPlayer")
        if bp:
            saved = unreal.EditorAssetLibrary.save_loaded_asset(bp)
    except Exception:
        pass
    # Read-back the live CDO values (diagnostic: confirms what the spawned pawn will use).
    current = {}
    for k in KNOWN:
        try:
            current[k] = cdo.get_editor_property(k)
        except Exception:
            pass
    return {"applied": applied, "skipped": skipped, "saved": saved, "current": current}

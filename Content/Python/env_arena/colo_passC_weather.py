"""Realism Pass C — weather/age the colosseum into a ruin: a major COLLAPSED SECTION on one
side (like the real Colosseum's missing outer wall) with the upper structure gone + a rubble
field, plus minor missing/leaning pieces elsewhere. Idempotent for its own rubble (Colo_Rub_*).
Destructive to geometry, so re-run colo_pass4/5/B to restore a pristine structure.
"""
import math
import unreal

MAP = "/Game/Maps/Arena_Ancient"
CUBE = "/Engine/BasicShapes/Cube"
TRAV = "/Game/Environments/AncientArena/M_Travertine"
A0, dA, N_TIERS = 6000.0, 360.0, 8
Aa = A0 + N_TIERS * dA
ZONE_LO, ZONE_HI = 20.0, 100.0      # collapsed arc (degrees)


def h(s):
    return sum(ord(c) for c in s)


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cube, trav = unreal.load_asset(CUBE), unreal.load_asset(TRAV)

    removed = leaned = 0
    for a in list(eas.get_all_level_actors()):
        lbl = a.get_actor_label()
        if lbl.startswith("Colo_Rub_"):
            eas.destroy_actor(a); continue
        if not (lbl.startswith("Colo_Geo_") or lbl.startswith("Colo_Det_")):
            continue
        loc = a.get_actor_location()
        th = math.degrees(math.atan2(loc.y, loc.x)) % 360.0
        in_zone = ZONE_LO <= th <= ZONE_HI
        is_arcade = any(t in lbl for t in ("Pier", "Arch", "Entab", "Col_", "Cap_", "Corn"))
        tier = -1
        if "Seat_" in lbl:
            try:
                tier = int(lbl.split("_")[3])
            except Exception:
                tier = -1
        if in_zone:
            if is_arcade or tier >= 3:        # upper structure collapsed here
                eas.destroy_actor(a); removed += 1
                continue
        else:
            r = h(lbl) % 100
            if is_arcade and r < 7:           # a few missing pieces elsewhere (broken teeth)
                eas.destroy_actor(a); removed += 1
                continue
            if r >= 90:                        # a few leaning pieces
                rot = a.get_actor_rotation()
                a.set_actor_rotation(unreal.Rotator(roll=rot.roll + (r % 9) - 4,
                                                    pitch=rot.pitch + (r % 7) - 3, yaw=rot.yaw), False)
                leaned += 1

    # --- rubble field in the collapsed zone ---
    n_rub = 0
    for n in range(22):
        f1 = (n * 37 % 100) / 100.0
        f2 = (n * 61 % 100) / 100.0
        thd = ZONE_LO + (ZONE_HI - ZONE_LO) * f1
        th = math.radians(thd)
        rad = A0 - 200.0 + (Aa - A0 + 600.0) * f2
        x, y = rad * math.cos(th), rad * math.sin(th)
        z = 90.0 + (n * 53 % 240)
        sc = 2.4 + (n * 29 % 100) / 28.0
        act = eas.spawn_actor_from_class(
            unreal.StaticMeshActor, unreal.Vector(x, y, z),
            unreal.Rotator(roll=(n * 47 % 70) - 35, pitch=(n * 31 % 60) - 30, yaw=n * 53 % 360))
        smc = act.get_component_by_class(unreal.StaticMeshComponent)
        if smc:
            smc.set_static_mesh(cube)
            smc.set_world_scale3d(unreal.Vector(sc, sc * 0.8, sc * 0.6))
            if trav:
                smc.set_material(0, trav)
        act.set_actor_label("Colo_Rub_%d" % n)
        n_rub += 1

    saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("[COLOC] removed=%d leaned=%d rubble=%d saved=%s" % (removed, leaned, n_rub, saved))
    unreal.log("[gate] RESULT=%s" % ("PASS" if saved else "FAIL"))


main()

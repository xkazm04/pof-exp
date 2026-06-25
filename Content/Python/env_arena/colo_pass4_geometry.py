"""Colosseum geometry — tiered seating cavea + Roman arcade, sized for the 4x (real-scale)
colosseum (~185 m diameter). Engine basic shapes + travertine. Idempotent (clears Colo_Geo_*).
Params A0/B0/N_TIERS/dA/dB/dZ/NP MUST match colo_pass5_arches.py.
"""
import math
import unreal

MAP = "/Game/Maps/Arena_Ancient"
CUBE = "/Engine/BasicShapes/Cube"
CYL = "/Engine/BasicShapes/Cylinder"
TRAV = "/Game/Environments/AncientArena/M_Travertine"

# --- real-scale form (shared with colo_pass5_arches) ---
A0, B0 = 6000.0, 5600.0       # inner seating semi-axes (outside the ~5800 square-arena corners)
N_TIERS = 8
dA, dB, dZ = 360.0, 330.0, 320.0
SEAT_W = 1150.0
NP = 40                       # arcade bays / arches
res = {"seats": 0, "arcade": 0}


def comp(a, c):
    return a.get_component_by_class(c)


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cube, cyl, trav = unreal.load_asset(CUBE), unreal.load_asset(CYL), unreal.load_asset(TRAV)

    cleared = 0
    for a in list(eas.get_all_level_actors()):
        if a.get_actor_label().startswith("Colo_Geo_"):
            eas.destroy_actor(a); cleared += 1

    def place(mesh, x, y, z, yaw, sx, sy, sz, label):
        act = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z),
                                         unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
        smc = comp(act, unreal.StaticMeshComponent)
        if smc and mesh:
            smc.set_static_mesh(mesh)
            smc.set_world_scale3d(unreal.Vector(sx, sy, sz))
            if trav:
                smc.set_material(0, trav)
        act.set_actor_label(label)

    # tiered seating cavea (rising + receding stepped rings)
    for i in range(N_TIERS):
        Ai, Bi, z = A0 + i * dA, B0 + i * dB, i * dZ
        ravg = (Ai + Bi) * 0.5
        segs = max(24, int(2.0 * math.pi * ravg / SEAT_W))
        seg_arc = 2.0 * math.pi * ravg / segs
        for k in range(segs):
            th = 2.0 * math.pi * k / segs
            place(cube, Ai * math.cos(th), Bi * math.sin(th), z + dZ * 0.5,
                  math.degrees(th) + 90.0, (seg_arc * 1.1) / 100.0, (dA * 1.3) / 100.0, dZ / 100.0,
                  "Colo_Geo_Seat_%d_%d" % (i, k))
            res["seats"] += 1

    # top arcade: piers + engaged columns + entablature
    Aa, Ba, top_z = A0 + N_TIERS * dA, B0 + N_TIERS * dB, N_TIERS * dZ
    for k in range(NP):
        th = 2.0 * math.pi * k / NP
        x, y, yaw = Aa * math.cos(th), Ba * math.sin(th), math.degrees(th) + 90.0
        place(cube, x, y, top_z + 380.0, yaw, 2.6, 3.6, 7.6, "Colo_Geo_Pier_%d" % k)
        place(cyl, (Aa - 180.0) * math.cos(th), (Ba - 180.0) * math.sin(th), top_z + 330.0,
              0.0, 2.2, 2.2, 6.6, "Colo_Geo_Col_%d" % k)
        res["arcade"] += 2
    ravg = (Aa + Ba) * 0.5
    esegs = max(28, int(2.0 * math.pi * ravg / 760.0))
    eseg = 2.0 * math.pi * ravg / esegs
    for k in range(esegs):
        th = 2.0 * math.pi * k / esegs
        place(cube, Aa * math.cos(th), Ba * math.sin(th), top_z + 800.0, math.degrees(th) + 90.0,
              (eseg * 1.1) / 100.0, 4.2, 1.6, "Colo_Geo_Entab_%d" % k)
        res["arcade"] += 1

    saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("[COLO4] cleared=%d seats=%d arcade=%d saved=%s" % (cleared, res["seats"], res["arcade"], saved))
    unreal.log("[gate] RESULT=%s" % ("PASS" if saved and res["seats"] > 0 else "FAIL"))


main()

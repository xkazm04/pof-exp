"""Colosseum — Pass 4: build the COLOSSEUM FORM procedurally (engine basic shapes + travertine).

Encircles the existing square arena with an elliptical, rising-and-receding tiered seating
cavea, topped by a Roman arcade (piers + entablature + a colonnade of columns). All
engine-native primitives, all travertine. Idempotent: clears prior Colo_* actors first.
"""
import math
import unreal

MAP = "/Game/Maps/Arena_Ancient"
CUBE = "/Engine/BasicShapes/Cube"
CYL = "/Engine/BasicShapes/Cylinder"
TRAV = "/Game/Environments/AncientArena/M_Travertine"

# --- form params ---
A0, B0 = 1500.0, 1350.0      # inner seating semi-axes (outside the ~1025 square arena corners)
N_TIERS = 6
dA, dB, dZ = 130.0, 120.0, 100.0
SEAT_W = 480.0               # target seating-block width along the arc
res = {"seats": 0, "arcade": 0, "errors": []}


def comp(a, c):
    return a.get_component_by_class(c)


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cube = unreal.load_asset(CUBE)
    cyl = unreal.load_asset(CYL)
    trav = unreal.load_asset(TRAV)

    # idempotent: clear prior colosseum geometry
    cleared = 0
    for a in list(eas.get_all_level_actors()):
        if a.get_actor_label().startswith("Colo_Geo_"):
            eas.destroy_actor(a); cleared += 1

    def place(mesh, x, y, z, yaw, sx, sy, sz, label, pitch=0.0, roll=0.0):
        act = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z),
                                         unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw))
        smc = comp(act, unreal.StaticMeshComponent)
        if smc and mesh:
            smc.set_static_mesh(mesh)
            smc.set_world_scale3d(unreal.Vector(sx, sy, sz))
            if trav:
                smc.set_material(0, trav)
        act.set_actor_label(label)
        return act

    # ---------- tiered seating cavea (elliptical, rising + receding) ----------
    idx = 0
    for i in range(N_TIERS):
        Ai = A0 + i * dA
        Bi = B0 + i * dB
        z = i * dZ
        ravg = (Ai + Bi) * 0.5
        segs = max(20, int(2.0 * math.pi * ravg / SEAT_W))
        seg_arc = 2.0 * math.pi * ravg / segs
        for k in range(segs):
            th = 2.0 * math.pi * k / segs
            x, y = Ai * math.cos(th), Bi * math.sin(th)
            yaw = math.degrees(th) + 90.0           # long axis tangent to the ring
            sx = (seg_arc * 1.12) / 100.0           # fill the arc (+overlap)
            sy = (dA * 1.35) / 100.0                # radial depth (tiers overlap)
            sz = 0.95                               # riser height ~95cm
            place(cube, x, y, z + 47.0, yaw, sx, sy, sz, "Colo_Geo_Seat_%d_%d" % (i, k))
            idx += 1
    res["seats"] = idx

    # ---------- top arcade: piers + entablature + colonnade ----------
    Aa = A0 + N_TIERS * dA
    Ba = B0 + N_TIERS * dB
    top_z = N_TIERS * dZ
    NP = 26
    for k in range(NP):
        th = 2.0 * math.pi * k / NP
        x, y = Aa * math.cos(th), Ba * math.sin(th)
        yaw = math.degrees(th) + 90.0
        # pier (tall box)
        place(cube, x, y, top_z + 260.0, yaw, 1.1, 1.6, 5.2, "Colo_Geo_Pier_%d" % k)
        # engaged column just inside the pier
        cx, cy = (Aa - 70.0) * math.cos(th), (Ba - 70.0) * math.sin(th)
        place(cyl, cx, cy, top_z + 230.0, 0.0, 0.85, 0.85, 4.6, "Colo_Geo_Col_%d" % k)
        res["arcade"] += 2
    # entablature ring on top of the piers
    ravg = (Aa + Ba) * 0.5
    esegs = max(24, int(2.0 * math.pi * ravg / 360.0))
    eseg = 2.0 * math.pi * ravg / esegs
    for k in range(esegs):
        th = 2.0 * math.pi * k / esegs
        x, y = Aa * math.cos(th), Ba * math.sin(th)
        yaw = math.degrees(th) + 90.0
        place(cube, x, y, top_z + 540.0, yaw, (eseg * 1.12) / 100.0, 2.0, 1.1,
              "Colo_Geo_Entab_%d" % k)
        res["arcade"] += 1

    saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("[COLO4] cleared=%d seats=%d arcade=%d saved=%s" % (cleared, res["seats"], res["arcade"], saved))
    unreal.log("[gate] RESULT=%s" % ("PASS" if saved and res["seats"] > 0 else "FAIL"))


main()

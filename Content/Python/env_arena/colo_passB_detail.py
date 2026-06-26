"""Realism Pass B — architectural detail: a podium wall ringing the arena floor, column
capitals + bases on the arcade colonnade, and a cornice/string-course band between the
seating and the arcade. Engine cubes + travertine. Idempotent (clears Colo_Det_*).
Params match colo_pass4_geometry.
"""
import math
import unreal

MAP = "/Game/Maps/Arena_Ancient"
CUBE = "/Engine/BasicShapes/Cube"
TRAV = "/Game/Environments/AncientArena/M_Travertine"
A0, B0, N_TIERS, dA, dB, dZ, NP = 6000.0, 5600.0, 8, 360.0, 330.0, 320.0, 40
Aa, Ba, TOP_Z = A0 + N_TIERS * dA, B0 + N_TIERS * dB, N_TIERS * dZ
res = {"pod": 0, "caps": 0, "corn": 0}


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cube, trav = unreal.load_asset(CUBE), unreal.load_asset(TRAV)

    for a in list(eas.get_all_level_actors()):
        if a.get_actor_label().startswith("Colo_Det_"):
            eas.destroy_actor(a)

    def place(x, y, z, yaw, sx, sy, sz, label):
        act = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z),
                                         unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
        smc = act.get_component_by_class(unreal.StaticMeshComponent)
        if smc:
            smc.set_static_mesh(cube)
            smc.set_world_scale3d(unreal.Vector(sx, sy, sz))
            if trav:
                smc.set_material(0, trav)
        act.set_actor_label(label)

    # --- podium wall ringing the arena floor (between floor and front-row seats) ---
    Ap, Bp = A0 - 250.0, B0 - 250.0
    ravg = (Ap + Bp) * 0.5
    psegs = max(40, int(2.0 * math.pi * ravg / 620.0))
    pseg = 2.0 * math.pi * ravg / psegs
    for k in range(psegs):
        th = 2.0 * math.pi * k / psegs
        place(Ap * math.cos(th), Bp * math.sin(th), 270.0, math.degrees(th) + 90.0,
              (pseg * 1.08) / 100.0, 1.6, 5.4, "Colo_Det_Pod_%d" % k)
        res["pod"] += 1

    # --- column capitals + bases on the arcade colonnade ---
    for k in range(NP):
        th = 2.0 * math.pi * k / NP
        cx, cy = (Aa - 180.0) * math.cos(th), (Ba - 180.0) * math.sin(th)
        yaw = math.degrees(th) + 90.0
        place(cx, cy, TOP_Z + 690.0, yaw, 3.3, 3.3, 0.9, "Colo_Det_Cap_%d" % k)   # capital
        place(cx, cy, TOP_Z + 40.0, yaw, 3.5, 3.5, 0.8, "Colo_Det_Base_%d" % k)   # base
        res["caps"] += 1

    # --- cornice/string-course band at the top of the seating (base of arcade) ---
    Ac, Bc = Aa - 60.0, Ba - 60.0
    cravg = (Ac + Bc) * 0.5
    csegs = max(48, int(2.0 * math.pi * cravg / 600.0))
    cseg = 2.0 * math.pi * cravg / csegs
    for k in range(csegs):
        th = 2.0 * math.pi * k / csegs
        place(Ac * math.cos(th), Bc * math.sin(th), TOP_Z - 20.0, math.degrees(th) + 90.0,
              (cseg * 1.08) / 100.0, 5.2, 0.7, "Colo_Det_Corn_%d" % k)
        res["corn"] += 1

    saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("[COLOB] podium=%d capitals/bases=%d cornice=%d saved=%s"
               % (res["pod"], res["caps"] * 2, res["corn"], saved))
    unreal.log("[gate] RESULT=%s" % ("PASS" if saved else "FAIL"))


main()

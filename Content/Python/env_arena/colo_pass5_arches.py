"""Colosseum arches — a rounded Roman arch (voussoirs + keystone) in every arcade bay,
sized for the 4x (real-scale) colosseum. Params MUST match colo_pass4_geometry.py.
Idempotent (clears Colo_Geo_Arch_*).
"""
import math
import unreal

MAP = "/Game/Maps/Arena_Ancient"
CUBE = "/Engine/BasicShapes/Cube"
TRAV = "/Game/Environments/AncientArena/M_Travertine"

A0, B0, N_TIERS, dA, dB, dZ = 6000.0, 5600.0, 8, 360.0, 330.0, 320.0
NP = 40
Aa = A0 + N_TIERS * dA
Ba = B0 + N_TIERS * dB
TOP_Z = N_TIERS * dZ
SPRING_Z = TOP_Z + 340.0
R_ARCH = 470.0
NVOUS = 11


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cube, trav = unreal.load_asset(CUBE), unreal.load_asset(TRAV)

    n = 0
    for a in list(eas.get_all_level_actors()):
        if a.get_actor_label().startswith("Colo_Geo_Arch_"):
            eas.destroy_actor(a); n += 1

    def place(x, y, z, rot, sx, sy, sz, label):
        act = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), rot)
        smc = act.get_component_by_class(unreal.StaticMeshComponent)
        if smc:
            smc.set_static_mesh(cube)
            smc.set_world_scale3d(unreal.Vector(sx, sy, sz))
            if trav:
                smc.set_material(0, trav)
        act.set_actor_label(label)

    built = 0
    for k in range(NP):
        thc = 2.0 * math.pi * (k + 0.5) / NP
        ct, st = math.cos(thc), math.sin(thc)
        tx, ty = -st, ct
        cx, cy = Aa * ct, Ba * st
        radial = unreal.Vector(ct, st, 0.0)
        for j in range(NVOUS):
            phi = math.radians(12.0 + 156.0 * j / (NVOUS - 1))
            cphi, sphi = math.cos(phi), math.sin(phi)
            px, py = cx + R_ARCH * cphi * tx, cy + R_ARCH * cphi * ty
            pz = SPRING_Z + R_ARCH * sphi
            r_hat = unreal.Vector(cphi * tx, cphi * ty, sphi)
            rot = unreal.MathLibrary.make_rot_from_zx(r_hat, radial)
            key = abs(phi - math.pi / 2.0) < 0.12
            place(px, py, pz, rot, 3.8, 1.5, 2.0 if key else 1.4, "Colo_Geo_Arch_%d_%d" % (k, j))
            built += 1

    saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("[COLO5] cleared=%d voussoirs=%d saved=%s" % (n, built, saved))
    unreal.log("[gate] RESULT=%s" % ("PASS" if saved and built > 0 else "FAIL"))


main()

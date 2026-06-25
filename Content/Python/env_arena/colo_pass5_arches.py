"""Colosseum — Pass 5: true ROUNDED ARCHES in each arcade bay (the iconic Roman feature).

For every bay between piers, lay a semicircle of voussoir blocks (+ a keystone) spanning the
opening, oriented in the bay's vertical plane via make_rot_from_zx. Engine cubes + travertine.
Idempotent: clears prior Colo_Geo_Arch_* first.
"""
import math
import unreal

MAP = "/Game/Maps/Arena_Ancient"
CUBE = "/Engine/BasicShapes/Cube"
TRAV = "/Game/Environments/AncientArena/M_Travertine"

# must match colo_pass4_geometry
A0, B0, N_TIERS, dA, dB, dZ = 1500.0, 1350.0, 6, 130.0, 120.0, 100.0
NP = 26
Aa = A0 + N_TIERS * dA
Ba = B0 + N_TIERS * dB
TOP_Z = N_TIERS * dZ
SPRING_Z = TOP_Z + 250.0
R_ARCH = 210.0
NVOUS = 11


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cube = unreal.load_asset(CUBE)
    trav = unreal.load_asset(TRAV)

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
        thc = 2.0 * math.pi * (k + 0.5) / NP    # opening center between piers
        ct, st = math.cos(thc), math.sin(thc)
        tx, ty = -st, ct                         # ring tangent (arch width axis, horizontal)
        cx, cy = Aa * ct, Ba * st
        radial = unreal.Vector(ct, st, 0.0)      # into the wall (box forward/X)
        for j in range(NVOUS):
            phi = math.radians(12.0 + 156.0 * j / (NVOUS - 1))
            cphi, sphi = math.cos(phi), math.sin(phi)
            px = cx + R_ARCH * cphi * tx
            py = cy + R_ARCH * cphi * ty
            pz = SPRING_Z + R_ARCH * sphi
            r_hat = unreal.Vector(cphi * tx, cphi * ty, sphi)   # arch-radius dir (box up/Z)
            rot = unreal.MathLibrary.make_rot_from_zx(r_hat, radial)
            key = abs(phi - math.pi / 2.0) < 0.12
            place(px, py, pz, rot, 2.1, 0.9, 0.85 if key else 0.62,
                  "Colo_Geo_Arch_%d_%d" % (k, j))
            built += 1

    saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("[COLO5] cleared=%d voussoirs=%d saved=%s" % (n, built, saved))
    unreal.log("[gate] RESULT=%s" % ("PASS" if saved and built > 0 else "FAIL"))


main()

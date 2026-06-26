"""Realism Pass D — ground & surroundings: rolling dune mounds (flattened sand spheres),
an outer DEBRIS FIELD of fallen travertine chunks + broken columns scattered around the
colosseum, and a denser dust haze for atmospheric depth. Idempotent (clears Colo_Dune_/Colo_Debris_).
"""
import math
import unreal

MAP = "/Game/Maps/Arena_Ancient"
SPHERE = "/Engine/BasicShapes/Sphere"
CUBE = "/Engine/BasicShapes/Cube"
CYL = "/Engine/BasicShapes/Cylinder"
SAND = "/Game/Environments/AncientArena/M_Sand_Floor"
TRAV = "/Game/Environments/AncientArena/M_Travertine"


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    sph, cube, cyl = unreal.load_asset(SPHERE), unreal.load_asset(CUBE), unreal.load_asset(CYL)
    sand, trav = unreal.load_asset(SAND), unreal.load_asset(TRAV)

    for a in list(eas.get_all_level_actors()):
        lbl = a.get_actor_label()
        if lbl.startswith("Colo_Dune_") or lbl.startswith("Colo_Debris_"):
            eas.destroy_actor(a)

    def spawn(mesh, x, y, z, rot, sx, sy, sz, mat, label):
        act = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), rot)
        smc = act.get_component_by_class(unreal.StaticMeshComponent)
        if smc:
            smc.set_static_mesh(mesh)
            smc.set_world_scale3d(unreal.Vector(sx, sy, sz))
            if mat:
                smc.set_material(0, mat)
        act.set_actor_label(label)

    # --- dune mounds: flattened sand spheres around the exterior ---
    n_dune = 0
    for n in range(46):
        th = math.radians((n * 78 + (n * n * 13) % 360) % 360)
        rad = 9500.0 + (n * 311 % 100) / 100.0 * 18000.0
        x, y = rad * math.cos(th), rad * math.sin(th)
        sx = 34.0 + (n * 17 % 100) / 100.0 * 46.0
        sy = sx * (0.6 + (n * 23 % 100) / 200.0)
        sz = 5.0 + (n * 29 % 100) / 100.0 * 7.0
        z = -(sz * 50.0) * 0.62        # half-buried mound
        spawn(sph, x, y, z, unreal.Rotator(0.0, 0.0, n * 41 % 360), sx, sy, sz, sand,
              "Colo_Dune_%d" % n)
        n_dune += 1

    # --- outer debris field: fallen travertine chunks + broken columns ---
    n_deb = 0
    for n in range(30):
        th = math.radians((n * 53 + 17) % 360)
        rad = 9300.0 + (n * 137 % 100) / 100.0 * 5200.0
        x, y = rad * math.cos(th), rad * math.sin(th)
        if n % 4 == 0:                  # broken column drum
            sc = 3.0 + (n * 19 % 100) / 50.0
            spawn(cyl, x, y, 60.0 + (n * 7 % 80),
                  unreal.Rotator(roll=90.0, pitch=0.0, yaw=n * 67 % 360),
                  sc * 0.5, sc * 0.5, sc, trav, "Colo_Debris_%d" % n)
        else:                            # rubble block
            sc = 2.2 + (n * 31 % 100) / 40.0
            spawn(cube, x, y, 40.0 + (n * 11 % 120),
                  unreal.Rotator(roll=(n * 43 % 60) - 30, pitch=(n * 29 % 50) - 25, yaw=n * 53 % 360),
                  sc, sc * 0.8, sc * 0.55, trav, "Colo_Debris_%d" % n)
        n_deb += 1

    # --- denser dust haze for distance depth ---
    for a in eas.get_all_level_actors():
        if a.get_class().get_name() == "ExponentialHeightFog":
            fc = a.get_component_by_class(unreal.ExponentialHeightFogComponent)
            try:
                fc.set_editor_property("fog_density", 0.03)
                fc.set_editor_property("volumetric_fog_extinction_scale", 2.0)
            except Exception:
                pass

    saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    unreal.log("[COLOD] dunes=%d debris=%d saved=%s" % (n_dune, n_deb, saved))
    unreal.log("[gate] RESULT=%s" % ("PASS" if saved else "FAIL"))


main()

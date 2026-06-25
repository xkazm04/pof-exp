"""Stream 1 (env-arena) — refinement pass on Arena_Ancient.

Fixes the ruin orientation bug (Rotator positional args put the spin into PITCH, tipping
every pillar over) and re-authors the ruins as a believable mix: standing broken columns
of varied height with a slight lean, a few deliberately fallen columns (read clearly as
ruins from the top-down cam), and scattered rubble. Also lifts the sky fill so the dark
arena walls aren't crushed to black. Re-saves the level.
"""
import math
import unreal

MAP = "/Game/Maps/Arena_Ancient"
PILLAR_MAT = "/Game/ArenaBuild/M_Arena_Pillar"
CYL = "/Engine/BasicShapes/Cylinder"
CUBE = "/Engine/BasicShapes/Cube"

res = {"deleted_old": 0, "standing": 0, "fallen": 0, "rubble": 0, "errors": []}


def log(m):
    unreal.log("[REFINE] " + m)


def comp(a, c):
    return a.get_component_by_class(c)


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cyl = unreal.load_asset(CYL)
    cube = unreal.load_asset(CUBE)
    pmat = unreal.load_asset(PILLAR_MAT)

    # --- arena extent for placement
    extent = unreal.Vector(900, 900, 300)
    dl = sl = None
    for a in list(eas.get_all_level_actors()):
        lbl = a.get_actor_label()
        cls = a.get_class().get_name()
        if lbl.startswith("Ruin_") or lbl.startswith("Rubble_"):
            eas.destroy_actor(a)
            res["deleted_old"] += 1
        elif lbl == "Arena":
            try:
                _, extent = a.get_actor_bounds(False)
            except Exception as e:  # noqa: BLE001
                res["errors"].append("bounds: %s" % e)
        elif cls == "DirectionalLight":
            dl = a
        elif cls == "SkyLight":
            sl = a
    radius = max(650.0, min(extent.x, extent.y) * 0.80)

    def mk(mesh, loc, rot, scale, label):
        act = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, rot)
        smc = comp(act, unreal.StaticMeshComponent)
        if smc and mesh:
            smc.set_static_mesh(mesh)
            smc.set_world_scale3d(scale)
            if pmat:
                smc.set_material(0, pmat)
        act.set_actor_label(label)
        return act

    # --- standing broken columns: ring around the arena, varied height + slight lean
    heights = [4.6, 2.0, 3.8, 1.4, 4.2, 2.6]  # z-scale; cylinder is 100uu tall, centered
    for i, sz in enumerate(heights):
        ang = math.radians(i * 60.0 + 15.0)
        cz = 50.0 * sz - 2.0  # base sits ~on the floor (centered-origin cylinder)
        loc = unreal.Vector(math.cos(ang) * radius, math.sin(ang) * radius, cz)
        lean = (-1.0 if i % 2 else 1.0) * (2.0 + (i % 3) * 2.0)
        rot = unreal.Rotator(roll=lean, pitch=0.0, yaw=i * 47.0)  # spin = YAW (was the bug)
        mk(cyl, loc, rot, unreal.Vector(0.6, 0.6, sz), "Ruin_Col_%02d" % i)
        res["standing"] += 1

    # --- deliberately fallen columns (roll 90 lays the long axis along the ground)
    fallen = [(0.55 * radius, -0.62 * radius, 33.0, 3.2),
              (-0.70 * radius, 0.30 * radius, 70.0, 2.6),
              (0.20 * radius, 0.75 * radius, 150.0, 2.2)]
    for j, (fx, fy, yaw, sz) in enumerate(fallen):
        cz = 50.0 * 0.6 + 4.0  # lying on its side: center ~ radius of the cylinder
        loc = unreal.Vector(fx, fy, cz)
        rot = unreal.Rotator(roll=90.0, pitch=0.0, yaw=yaw)
        mk(cyl, loc, rot, unreal.Vector(0.6, 0.6, sz), "Ruin_Fallen_%02d" % j)
        res["fallen"] += 1

    # --- scattered rubble blocks near the ring
    rubble = [(0.85, 20.0), (0.78, 75.0), (0.90, 130.0), (0.82, 200.0), (0.88, 290.0)]
    for k, (rr, deg) in enumerate(rubble):
        ang = math.radians(deg)
        sc = 0.35 + (k % 3) * 0.15
        loc = unreal.Vector(math.cos(ang) * radius * rr, math.sin(ang) * radius * rr, sc * 50.0)
        rot = unreal.Rotator(roll=k * 11.0, pitch=k * 7.0, yaw=deg)
        mk(cube, loc, rot, unreal.Vector(sc, sc * 1.3, sc), "Rubble_%02d" % k)
        res["rubble"] += 1

    # --- lift the ambient so the dark walls read (sky fill up; key a touch up)
    if sl:
        slc = comp(sl, unreal.SkyLightComponent)
        slc.set_intensity(1.6)
        slc.set_light_color(unreal.LinearColor(0.78, 0.80, 0.88, 1.0))
    if dl:
        comp(dl, unreal.DirectionalLightComponent).set_intensity(3.5)

    saved = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    res["saved"] = bool(saved)
    log("deleted=%d standing=%d fallen=%d rubble=%d radius=%.0f saved=%s"
        % (res["deleted_old"], res["standing"], res["fallen"], res["rubble"], radius, saved))
    log("[gate] RESULT=%s" % ("PASS" if saved else "FAIL"))


main()

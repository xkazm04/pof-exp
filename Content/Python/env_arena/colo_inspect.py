"""Colosseum — quick scene inspect. Logs (no JSON parsing needed) what the integrator
left in Arena_Ancient: actors near center, anything with a default/grid material (the
checkerboard cylinder), and the player/skeletal/BP actors. Markers: [INS]."""
import unreal

MAP = "/Game/Maps/Arena_Ancient"


def main():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = list(eas.get_all_level_actors())
    unreal.log("[INS] total actors=%d" % len(actors))

    for a in actors:
        cls = a.get_class().get_name()
        lbl = a.get_actor_label()
        loc = a.get_actor_location()
        near = abs(loc.x) < 350 and abs(loc.y) < 350
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        mat0 = None
        mesh = None
        if smc:
            m = smc.get_editor_property("static_mesh")
            mesh = m.get_name() if m else None
            try:
                mats = smc.get_materials()
                if mats and mats[0]:
                    mat0 = mats[0].get_name()
            except Exception:
                pass
        is_default = mat0 in ("WorldGridMaterial", "DefaultMaterial", "M_Grid", "BasicShapeMaterial")
        # Flag: near center, OR default/grid material, OR player/character/BP/skeletal actors
        flag = near or is_default or cls.startswith("BP_") or "Character" in cls or "Player" in cls
        if a.get_component_by_class(unreal.SkeletalMeshComponent):
            flag = True
        if flag:
            unreal.log("[INS] %-26s cls=%-22s loc=(%.0f,%.0f,%.0f) mesh=%s mat0=%s%s"
                       % (lbl, cls, loc.x, loc.y, loc.z, mesh, mat0,
                          "  <<DEFAULT_MAT" if is_default else ""))
    unreal.log("[INS] DONE")
    unreal.log("[gate] RESULT=PASS")


main()

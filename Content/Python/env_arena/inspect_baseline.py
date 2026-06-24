"""Stream 1 (env-arena) — baseline inspection of Arena_Ancient.

Loads /Game/Maps/Arena_Ancient and dumps a structured summary of what the map
currently contains (it is a Phase-0 duplicate of VerticalSlice): every actor with
class/label/location, static-mesh actors with their mesh + assigned materials, and
the presence of the key environment actors (lights, fog, post-process, PlayerStart,
navmesh). Writes JSON to Saved/env_baseline.json for the agent to read back, and
prints [INSPECT] marker lines to the log.
"""
import json
import unreal

MAP_PKG = "/Game/Maps/Arena_Ancient"
OUT = unreal.Paths.project_saved_dir() + "env_baseline.json"


def loc_of(a):
    v = a.get_actor_location()
    return [round(v.x, 1), round(v.y, 1), round(v.z, 1)]


def main():
    unreal.log("[INSPECT] BEGIN load %s" % MAP_PKG)
    unreal.EditorLoadingAndSavingUtils.load_map(MAP_PKG)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    actors = eas.get_all_level_actors()

    summary = {
        "map": MAP_PKG,
        "actor_count": len(actors),
        "actors": [],
        "static_meshes": [],
        "key_actors": {
            "DirectionalLight": 0, "SkyLight": 0, "ExponentialHeightFog": 0,
            "PostProcessVolume": 0, "PlayerStart": 0, "NavMeshBoundsVolume": 0,
            "SkyAtmosphere": 0, "RecastNavMesh": 0,
        },
    }

    for a in actors:
        cls = a.get_class().get_name()
        try:
            label = a.get_actor_label()
        except Exception:
            label = "?"
        summary["actors"].append({"class": cls, "label": label, "loc": loc_of(a)})
        for k in summary["key_actors"]:
            if k in cls:
                summary["key_actors"][k] += 1

        # Static-mesh actors: capture mesh + materials (+ usage flags on each material)
        smc = a.get_component_by_class(unreal.StaticMeshComponent)
        if smc:
            mesh = smc.get_editor_property("static_mesh")
            mats = []
            try:
                mat_ifaces = smc.get_materials()
            except Exception:
                mat_ifaces = []
            for m in mat_ifaces:
                if not m:
                    mats.append(None)
                    continue
                entry = {"name": m.get_name(), "path": m.get_path_name()}
                base = m
                if isinstance(m, unreal.MaterialInstance):
                    try:
                        base = m.get_base_material()
                    except Exception:
                        base = None
                if isinstance(base, unreal.Material):
                    for flag in ("used_with_static_lighting", "used_with_skeletal_mesh"):
                        try:
                            entry[flag] = base.get_editor_property(flag)
                        except Exception:
                            entry[flag] = "?"
                mats.append(entry)
            summary["static_meshes"].append({
                "label": label,
                "mesh": mesh.get_path_name() if mesh else None,
                "loc": loc_of(a),
                "scale": [round(x, 2) for x in (a.get_actor_scale3d().x, a.get_actor_scale3d().y, a.get_actor_scale3d().z)],
                "materials": mats,
            })

    with open(OUT, "w") as f:
        json.dump(summary, f, indent=2)

    unreal.log("[INSPECT] actor_count=%d static_meshes=%d" % (summary["actor_count"], len(summary["static_meshes"])))
    unreal.log("[INSPECT] key_actors=%s" % json.dumps(summary["key_actors"]))
    unreal.log("[INSPECT] wrote %s" % OUT)
    unreal.log("[gate] RESULT=PASS")


main()

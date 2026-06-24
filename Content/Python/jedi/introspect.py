"""
introspect.py — Stream 2 (Jedi character) reconnaissance.

Doubles as a smoke test for the worktree editor (copied binaries) and a report
of how the existing player + weapon are configured, so BP_JediPlayer can be
authored to match the real base-class contract instead of guessing.

Writes a JSON sidecar to Saved/jedi_introspect.json (verifiable post-run).

Run headless:
  UnrealEditor-Cmd.exe <PoF-jedi/PoF.uproject> -run=pythonscript \
    -script="<abs path to this file>" -unattended -nopause -nosplash -NoLiveCoding
"""
import json
import unreal

asset_lib = unreal.EditorAssetLibrary
OUT = {}


def _log(msg):
    unreal.log_warning("[jedi.introspect] " + str(msg))


def exists(path):
    return bool(asset_lib.does_asset_exist(path))


def load(path):
    return asset_lib.load_asset(path) if exists(path) else None


def subobjects(bp):
    """Return list of (name, class_name, is_inherited, obj) for a Blueprint."""
    sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = sub.k2_gather_subobject_data_for_blueprint(bp)
    bfl = unreal.SubobjectDataBlueprintFunctionLibrary
    out = []
    for h in handles or []:
        data = sub.k2_find_subobject_data_from_handle(h)
        if data is None:
            continue
        obj = bfl.get_associated_object(data)
        if obj is None:
            continue
        try:
            inh = bfl.is_inherited_component(data)
        except Exception:
            inh = None
        out.append((obj.get_name(), obj.get_class().get_name(), inh, obj))
    return out


def describe_bp(path):
    info = {"path": path, "exists": exists(path)}
    if not info["exists"]:
        return info
    bp = load(path)
    try:
        gen = bp.generated_class()
        info["generated_class"] = gen.get_name() if gen else None
        parent = bp.get_editor_property("parent_class")
        info["parent_class"] = parent.get_name() if parent else None
    except Exception as e:
        info["class_err"] = str(e)
    comps = []
    for name, cls, inh, obj in subobjects(bp):
        c = {"name": name, "class": cls, "inherited": inh}
        # Skeletal mesh component detail
        if isinstance(obj, unreal.SkeletalMeshComponent):
            try:
                sk = obj.get_editor_property("skeletal_mesh_asset")
                c["skeletal_mesh"] = sk.get_path_name() if sk else None
            except Exception:
                c["skeletal_mesh"] = "?"
            try:
                ac = obj.get_editor_property("anim_class")
                c["anim_class"] = ac.get_path_name() if ac else None
            except Exception:
                c["anim_class"] = "?"
            try:
                ov = obj.get_editor_property("override_materials")
                c["override_materials"] = [m.get_path_name() if m else None for m in (ov or [])]
            except Exception:
                c["override_materials"] = "?"
        # Static mesh component detail (the WeaponMesh)
        if isinstance(obj, unreal.StaticMeshComponent):
            try:
                sm = obj.get_editor_property("static_mesh")
                c["static_mesh"] = sm.get_path_name() if sm else None
            except Exception:
                c["static_mesh"] = "?"
            try:
                ov = obj.get_editor_property("override_materials")
                c["override_materials"] = [m.get_path_name() if m else None for m in (ov or [])]
            except Exception:
                pass
            try:
                rt = obj.get_editor_property("relative_location")
                c["rel_loc"] = [rt.x, rt.y, rt.z]
            except Exception:
                pass
        comps.append(c)
    info["components"] = comps
    return info


def main():
    _log("=== INTROSPECT START ===")
    OUT["engine_version"] = unreal.SystemLibrary.get_engine_version()

    # Candidate player/base blueprints.
    for p in [
        "/Game/Characters/Jedi/BP_JediPlayer",
        "/Game/VerticalSlice/BP_VSPlayer",
        "/Game/VerticalSlice/BP_VSEnemy",
    ]:
        key = p.rsplit("/", 1)[-1]
        OUT[key] = describe_bp(p)

    # Key assets the Jedi will reference.
    OUT["assets"] = {}
    for a in [
        "/MoverTests/Characters/Mannequins/Meshes/SKM_Manny",
        "/Game/Characters/Player/ABP_VSPlayer",
        "/Game/Characters/Player/PA_VSPlayer",
        "/Game/FX/M_Saber_Blue",
        "/Game/FX/M_Sith_Body",
        "/Engine/BasicShapes/Cylinder",
        "/Engine/BasicShapes/Cone",
    ]:
        OUT["assets"][a] = exists(a)

    # SKM_Manny material slot count (so robe override knows how many slots).
    skm = load("/MoverTests/Characters/Mannequins/Meshes/SKM_Manny")
    if skm:
        try:
            mats = skm.get_editor_property("materials")
            OUT["skm_manny_slots"] = [
                (m.get_slot_name().__str__() if hasattr(m, "get_slot_name") else str(i))
                for i, m in enumerate(mats or [])
            ]
            OUT["skm_manny_slot_count"] = len(mats or [])
        except Exception as e:
            OUT["skm_manny_err"] = str(e)

    out_path = unreal.Paths.combine([unreal.Paths.project_saved_dir(), "jedi_introspect.json"])
    with open(out_path, "w", encoding="utf-8") as fh:
        json.dump(OUT, fh, indent=2)
    _log("wrote " + out_path)
    _log("[gate] RESULT=PASS")


main()

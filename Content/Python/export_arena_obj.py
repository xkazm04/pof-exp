"""
export_arena_obj.py
===================
Exports /Game/ArenaBuild/SM_Arena to a UV-mapped OBJ at
Content/ArenaBuild/Arena.obj for Leonardo's 3D-texture endpoint (OBJ + UVs
required). Does NOT touch build_arena.py / build_arena_ue.py.

Run headless: -ExecutePythonScript=<abs path>.
"""

import os
import unreal

SM_ARENA_PATH = "/Game/ArenaBuild/SM_Arena"
OUT_OBJ = os.path.normpath(os.path.join(
    unreal.Paths.project_dir(), "Content", "ArenaBuild", "Arena.obj"))

asset_lib = unreal.EditorAssetLibrary


def _log(msg):
    unreal.log("[export_arena_obj] " + msg)


def main():
    _log("=== export START ===")
    mesh = asset_lib.load_asset(SM_ARENA_PATH)
    if mesh is None:
        raise RuntimeError("SM_Arena missing at " + SM_ARENA_PATH)

    task = unreal.AssetExportTask()
    task.set_editor_property("object", mesh)
    task.set_editor_property("filename", OUT_OBJ)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_identical", True)
    task.set_editor_property("prompt", False)
    task.set_editor_property("exporter", unreal.StaticMeshExporterOBJ())
    ok = unreal.Exporter.run_asset_export_task(task)
    if not ok or not os.path.isfile(OUT_OBJ):
        raise RuntimeError("OBJ export failed; expected " + OUT_OBJ)
    _log("Exported OBJ: %s (%d bytes)" % (OUT_OBJ, os.path.getsize(OUT_OBJ)))
    _log("=== export COMPLETE ===")


if __name__ == "__main__":
    main()

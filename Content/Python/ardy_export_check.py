"""Export the retargeted ARDY->Manny AnimSequences to FBX for external visual review."""
import unreal

OUT_DIR = r"C:\Users\kazda\kiro\ardy\outputs\manny"
SRC = "/Game/Generated/Ardy/Manny"
CLIPS = ["slash_Anim_Manny", "run_Anim_Manny", "roll_Anim_Manny", "idle_Anim_Manny"]

for name in CLIPS:
    seq = unreal.EditorAssetLibrary.load_asset(f"{SRC}/{name}")
    if not seq:
        print(f"POF_EXPORT_FAIL {name}: not found")
        continue
    opts = unreal.FbxExportOption()
    opts.ascii = False
    opts.export_preview_mesh = True  # needs a real RHI: run with -RenderOffScreen, NOT -nullrhi
    task = unreal.AssetExportTask()
    task.object = seq
    task.filename = f"{OUT_DIR}\\{name}.fbx"
    task.automated = True
    task.replace_identical = True
    task.options = opts
    task.exporter = unreal.AnimSequenceExporterFBX()
    ok = unreal.Exporter.run_asset_export_task(task)
    print(f"POF_EXPORT {name} ok={ok}")
print("POF_EXPORT_DONE")

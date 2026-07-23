"""Is BP_VSPlayerController a child of AARPGPlayerController? (F-binding root cause)"""
import unreal

cls = unreal.EditorAssetLibrary.load_blueprint_class('/Game/VerticalSlice/BP_VSPlayerController')
if cls:
    is_arpg = unreal.MathLibrary.class_is_child_of(cls, unreal.ARPGPlayerController.static_class())
    is_pc = unreal.MathLibrary.class_is_child_of(cls, unreal.PlayerController.static_class())
    unreal.log(f"[pc-inspect] child_of ARPGPlayerController={is_arpg} child_of PlayerController={is_pc}")
    # also list which input-related properties the BP sets
    cdo = unreal.get_default_object(cls)
    unreal.log(f"[pc-inspect] CDO class = {cdo.get_class().get_path_name()}")
else:
    unreal.log("[pc-inspect] NOT FOUND")
unreal.log("[pc-inspect] RESULT done")

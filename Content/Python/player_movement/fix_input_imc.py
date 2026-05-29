"""Fix IMC_VerticalSlice IA_Move WASD modifiers. Delegates to the C++
UPoFAnimBPAuthoringLibrary.SetupWASDMoveModifiers — Python-authored InputModifier objects
serialize but stay INERT at runtime; C++ NewObject(IMC)+MapKey produces working instanced
modifiers. W=Swizzle(YXZ) fwd, S=Swizzle+Negate back, A=Negate left, D=none right."""
import unreal
def run(args):
    imc = unreal.EditorAssetLibrary.load_asset("/Game/Input/IMC_VerticalSlice")
    ia = unreal.EditorAssetLibrary.load_asset("/Game/Input/Actions/IA_Move")
    if not imc or not ia:
        return {"failed": "load failed"}
    ok = unreal.PoFAnimBPAuthoringLibrary.setup_wasd_move_modifiers(imc, ia)
    saved = unreal.EditorAssetLibrary.save_asset("/Game/Input/IMC_VerticalSlice")
    return {"ok": bool(ok), "saved": bool(saved)}

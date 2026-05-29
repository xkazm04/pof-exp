"""Fix IMC_VerticalSlice IA_Move bindings.

W/S/A/D were all bound to IA_Move (Vector2D) with NO modifiers, so each dumped +1.0
onto X -> all four moved +X ("only D works"). Apply the canonical third-person pattern:
  W = SwizzleAxis(YXZ)           -> (0,+1) forward
  S = SwizzleAxis(YXZ) + Negate  -> (0,-1) backward
  A = Negate                     -> (-1,0) left
  D = (none)                     -> (+1,0) right

Rebuilds the mappings array with FRESH structs (mutating get_editor_property copies in
place does NOT persist). Verify behaviourally with RunScenarioEx (real key W -> forward).
"""
import unreal

IMC_PATH = "/Game/Input/IMC_VerticalSlice"
PLAN = {"W": ["swizzle"], "S": ["swizzle", "negate"], "A": ["negate"], "D": []}


def run(args):
    imc = unreal.EditorAssetLibrary.load_asset(IMC_PATH)
    if not imc:
        return {"failed": f"IMC not found at {IMC_PATH}"}

    def make_mods(tags):
        out = []
        for tag in tags:
            if tag == "swizzle":
                s = unreal.new_object(unreal.InputModifierSwizzleAxis, imc)
                s.set_editor_property("order", unreal.InputAxisSwizzle.YXZ)
                out.append(s)
            else:
                out.append(unreal.new_object(unreal.InputModifierNegate, imc))
        return out

    old = imc.get_editor_property("mappings")
    new_maps = []
    applied = []
    for m in old:
        act = m.get_editor_property("action")
        key = m.get_editor_property("key")
        kn = str(key.get_editor_property("key_name")) if key else ""
        nm = unreal.EnhancedActionKeyMapping()
        nm.set_editor_property("action", act)
        nm.set_editor_property("key", key)
        if act and act.get_name() == "IA_Move" and kn in PLAN:
            mods = make_mods(PLAN[kn])
            nm.set_editor_property("modifiers", mods)
            applied.append(f"{kn}:{[x.get_class().get_name() for x in mods]}")
        else:
            nm.set_editor_property("modifiers", m.get_editor_property("modifiers"))
        new_maps.append(nm)

    imc.set_editor_property("mappings", new_maps)
    saved = unreal.EditorAssetLibrary.save_asset(IMC_PATH)

    # Read back from the in-memory asset to confirm persistence.
    check = []
    for m in imc.get_editor_property("mappings"):
        act = m.get_editor_property("action")
        key = m.get_editor_property("key")
        if act and act.get_name() == "IA_Move":
            mods = [x.get_class().get_name() for x in (m.get_editor_property("modifiers") or []) if x]
            check.append(f"{key.get_editor_property('key_name')}:{mods}")
    return {"applied": applied, "saved": bool(saved), "readback": check}

"""
wire_combat_abilities.py
========================
Idempotent: grants the player ability roster, populates the hotbar loadout, and
authors the slot/dodge input assets + IMC mappings + controller input assignments.
Run headless AFTER the C++ (editable AbilityLoadout) is compiled:

    "C:\\Program Files\\Epic Games\\UE_5.7\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
        "C:\\Users\\kazda\\Documents\\Unreal Projects\\PoF\\PoF.uproject" ^
        -run=pythonscript -script="<abs path to this file>" -unattended -nopause
"""

import unreal

ACTIONS_DIR = "/Game/Input/Actions"
IA_ATTACK = ACTIONS_DIR + "/IA_Attack"          # Digital(bool) template to duplicate
IMC_PATH = "/Game/Input/IMC_VerticalSlice"
BP_VSPLAYER = "/Game/VerticalSlice/BP_VSPlayer"
BP_VSPLAYERCTRL = "/Game/VerticalSlice/BP_VSPlayerController"

# (asset name, controller UPROPERTY name, FKey name)
SLOT_INPUTS = [
    ("IA_AbilitySlot1", "IA_AbilitySlot1", "One"),
    ("IA_AbilitySlot2", "IA_AbilitySlot2", "Two"),
    ("IA_AbilitySlot3", "IA_AbilitySlot3", "Three"),
    ("IA_AbilitySlot4", "IA_AbilitySlot4", "Four"),
    ("IA_Dodge",        "IA_Dodge",        "SpaceBar"),
]

GRANT_CLASSES = [
    "/Script/PoF.GA_Fireball", "/Script/PoF.GA_GroundSlam",
    "/Script/PoF.GA_DashStrike", "/Script/PoF.GA_WarCry", "/Script/PoF.GA_Dodge",
]
LOADOUT = [
    (0, "/Script/PoF.GA_Fireball"),
    (1, "/Script/PoF.GA_GroundSlam"),
    (2, "/Script/PoF.GA_DashStrike"),
    (3, "/Script/PoF.GA_WarCry"),
]

asset_lib = unreal.EditorAssetLibrary


def load_class(path):
    cls = unreal.load_class(None, path)
    if cls is None:
        cls = unreal.load_class(None, path + "_C")
    return cls


def ensure_input_actions():
    actions = {}
    for name, _prop, _key in SLOT_INPUTS:
        path = ACTIONS_DIR + "/" + name
        if not asset_lib.does_asset_exist(path):
            if asset_lib.duplicate_asset(IA_ATTACK, path) is None:
                raise RuntimeError("Failed to duplicate IA_Attack -> " + path)
            unreal.log("[wire_combat] created " + path)
        actions[name] = asset_lib.load_asset(path)
    return actions


def make_key(name):
    k = unreal.Key()
    k.set_editor_property("key_name", unreal.Name(name))
    return k


def ensure_imc_mappings(actions):
    imc = asset_lib.load_asset(IMC_PATH)
    if imc is None:
        raise RuntimeError("IMC missing: " + IMC_PATH)
    dkm = imc.get_editor_property("default_key_mappings")
    mappings = list(dkm.get_editor_property("mappings"))
    existing = set()
    for m in mappings:
        a = m.get_editor_property("action")
        if a:
            existing.add(a.get_path_name())
    for name, _prop, key in SLOT_INPUTS:
        act = actions[name]
        if act.get_path_name() in existing:
            continue
        m = unreal.EnhancedActionKeyMapping()
        m.set_editor_property("action", act)
        m.set_editor_property("key", make_key(key))
        mappings.append(m)
        unreal.log("[wire_combat] mapped " + name + " -> " + key)
    dkm.set_editor_property("mappings", mappings)
    imc.set_editor_property("default_key_mappings", dkm)
    asset_lib.save_asset(IMC_PATH)


def wire_player():
    bp = asset_lib.load_asset(BP_VSPLAYER)
    cdo = unreal.get_default_object(bp.generated_class())

    current = list(cdo.get_editor_property("DefaultAbilities"))
    current_paths = {c.get_path_name() for c in current if c}
    for path in GRANT_CLASSES:
        cls = load_class(path)
        if cls is None:
            raise RuntimeError("Missing ability class " + path)
        if cls.get_path_name() not in current_paths:
            current.append(cls)
            current_paths.add(cls.get_path_name())
    cdo.set_editor_property("DefaultAbilities", current)

    loadout = {}
    for slot, path in LOADOUT:
        cls = load_class(path)
        if cls is None:
            raise RuntimeError("Missing loadout class " + path)
        loadout[slot] = cls
    # Full-replace (not merge): the slice's default loadout is wholly owned by this script.
    cdo.set_editor_property("AbilityLoadout", loadout)

    asset_lib.save_asset(BP_VSPLAYER)
    unreal.log("[wire_combat] BP_VSPlayer: granted %d, loadout %d slots" % (len(current), len(loadout)))


def wire_controller(actions):
    bp = asset_lib.load_asset(BP_VSPLAYERCTRL)
    cdo = unreal.get_default_object(bp.generated_class())
    for name, prop, _key in SLOT_INPUTS:
        cdo.set_editor_property(prop, actions[name])
    asset_lib.save_asset(BP_VSPLAYERCTRL)
    unreal.log("[wire_combat] BP_VSPlayerController inputs assigned")


def main():
    actions = ensure_input_actions()
    ensure_imc_mappings(actions)
    wire_player()
    wire_controller(actions)
    unreal.log("[wire_combat] done")


main()

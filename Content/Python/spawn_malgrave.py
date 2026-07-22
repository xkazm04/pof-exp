"""Spawn Darth Malgrave (the Duel Challenge speaker) into the CURRENT level.

Run from the interactive editor's Python console (Output Log → Cmd, or
Tools → Execute Python Script) with any map open — VerticalSlice recommended:

    py "C:/Users/kazda/Documents/Unreal Projects/PoF/Content/Python/spawn_malgrave.py"

Places an AARPGNPCActor near the player start with NPCID=Malgrave; its
BeginPlay self-builds the dialog-duel-intro tree (ARPGDuelIntroDialogue).
PIE → walk up → press F to talk. Idempotent: re-running moves the existing actor.
"""
import unreal

LABEL = "NPC_DarthMalgrave"
OFFSET = unreal.Vector(400.0, 150.0, 0.0)  # a few steps in front of the player start

ell = unreal.EditorLevelLibrary

# find a player start to anchor the spawn
anchor = unreal.Vector(0.0, 0.0, 100.0)
for a in ell.get_all_level_actors():
    if isinstance(a, unreal.PlayerStart):
        anchor = a.get_actor_location()
        break

pos = unreal.Vector(anchor.x + OFFSET.x, anchor.y + OFFSET.y, anchor.z)

# idempotent: reuse an existing Malgrave if present
existing = [a for a in ell.get_all_level_actors() if a.get_actor_label() == LABEL]
if existing:
    npc = existing[0]
    npc.set_actor_location(pos, False, False)
    unreal.log(f"[spawn_malgrave] moved existing {LABEL} to {pos}")
else:
    npc_class = unreal.load_class(None, "/Script/PoF.ARPGNPCActor")
    npc = ell.spawn_actor_from_class(npc_class, pos)
    npc.set_actor_label(LABEL)
    unreal.log(f"[spawn_malgrave] spawned {LABEL} at {pos}")

# property-name reflection differs per UE version — try the common spellings
for prop, val in (("npcid", "Malgrave"), ("npc_id", "Malgrave")):
    try:
        npc.set_editor_property(prop, val)
        break
    except Exception:
        continue
try:
    npc.set_editor_property("display_name", unreal.Text("Darth Malgrave"))
except Exception:
    pass
# face back toward the player start
look = unreal.MathLibrary.find_look_at_rotation(pos, anchor)
npc.set_actor_rotation(unreal.Rotator(0.0, look.yaw, 0.0), False)

unreal.log("[spawn_malgrave] done — PIE, walk up, press F to talk. Save the map to keep him.")

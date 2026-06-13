# Know-Before-Act protocol (the discipline that pairs with the verification gate)

The verification gate (`verify_gate.py`) catches a bad change *after* it happens. This protocol
stops me acting on an unverified assumption *before* it happens. Together they close both ends.

**Rule: never act on a property of the engine I have not queried this session.** If a change
depends on "X is true" (an axis, a transform, whether a clip travels, a default value), I PROBE
X first and confirm it. Assumptions are declared and confirmed, not trusted.

## Pre-action ledger (write before any non-trivial change)

```
ACTION:      <what I'm about to do>
ASSUMPTIONS: <each fact the action depends on>
PROBE:       <the query that confirms each assumption>  ->  RESULT: <confirmed | WRONG -> revise>
```
If any assumption can't be confirmed by a probe, I either build the probe or treat the action as
unsafe and stop.

## Probe catalog (`observation/probe.py`, bridge module)

| Probe | Answers (the assumption it kills) | Call |
|-------|-----------------------------------|------|
| `root_motion(asset[, root_bone])` | Does this clip ACTUALLY travel, how far, along which bone axis? Kills "I assume it moves" + "I assume +X is forward". Reports `DROPPED` when a retarget stripped root motion. | `{module:"observation.probe",function:"root_motion",args:{asset:"/Game/..."}}` |
| `mesh_orientation(class)` | The character mesh yaw offset = how bone-space maps to ACTOR space. Kills "bone +X == actor forward". | `{...,function:"mesh_orientation",args:{class:"/Game/VerticalSlice/BP_VSPlayer.BP_VSPlayer_C"}}` |

Add a probe here the moment an assumption bites — that's how the catalog grows.

## Verified facts banked from this project (consult before re-deriving)

- **BP_VSPlayer mesh yaw = -90** → to move the actor forward (+X), root motion must run along
  **bone +Y**. (Baking along bone +X sends the roll sideways to actor -Y.)
- **The Mixamo->Manny retarget (RTG_MixamoToManny) DROPS root motion** → any retargeted clip reads
  `enable_root_motion=false`, delta ~0. It will not travel without a bake (or a retargeter
  configured to transfer root motion). Probe `root_motion` confirms in one call.

## Required probes before common actions

- **Before baking/editing root motion:** `root_motion(clip)` (travels? which axis?) +
  `mesh_orientation` (bone->actor mapping). Bake along the actor-forward axis, not an assumed one.
- **Before relying on a clip to drive movement:** `root_motion(clip)` — if `DROPPED`, it won't move.
- **Before setting/trusting any CDO/asset value:** read it back (Remote Control `/property` or a probe),
  don't assume the .h default reached the instance.

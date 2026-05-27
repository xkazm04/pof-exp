# player_movement

Pipeline modules for the player-movement catalog row (Tier-2 Mixamo).

Each module exposes `run(args: dict) -> dict` and is idempotent.

## Return envelope

```json
{
  "created": ["asset_name", ...],
  "updated": ["asset_name", ...],
  "skipped": ["asset_name", ...],
  "failed":  ["human-readable error", ...]
}
```

## Modules

| Module | Purpose | Step |
|---|---|---|
| `verify_mesh`        | Verify `BP_VSPlayer.Mesh = SKM_Manny`, capsule sized, IA/IMC present | 01 |
| `import_clips`       | Batch FBX import from `Content/Source/Mixamo/Raw/` → `/Game/Mixamo/Raw/` | 03 |
| `build_ik_rigs`      | `IK_Mixamo` + `IK_Manny` + `RTG_MixamoToManny`                       | 04 |
| `retarget`           | Batch retarget Mixamo clips → `/Game/Mixamo/Retargeted/SKM_Manny/`  | 05 |
| `build_blend_space`  | Wire 11 retargeted clips into `BS_Locomotion` sample grid           | 06 |
| `build_anim_bp`      | Author `ABP_VSPlayer` via `PoFAnimBPAuthoringLibrary`              | 08 |
| `build_montage`      | Build `AM_Roll` from `Forward_Roll_RT` + iframe notify              | 09 |
| `build_test_level`   | Build `TestLevel_PlayerMovement.umap` for the L4 gate               | 10 |

## Mixamo source files

Drop these 10 FBX files into `Content/Source/Mixamo/Raw/` (gitignored):

- `Standard_Idle.fbx` (with skin — brings the X Bot rig)
- `Walking.fbx`, `Walking_Backwards.fbx`
- `Left_Strafe_Walking.fbx`, `Right_Strafe_Walking.fbx`
- `Running.fbx`, `Running_Backward.fbx`
- `Left_Strafe.fbx`, `Right_Strafe.fbx`
- `Forward_Roll.fbx` (without "In Place" — root motion drives the dodge)

Mixamo settings: FBX Binary, 30 fps, character = X Bot, all "In Place" ON
except Forward_Roll.

## Invocation

Modules are called by the lab pipeline UI via the `/pof/python/run` bridge route:

```
POST http://localhost:30040/pof/python/run
{"module": "player_movement.import_clips", "function": "run", "args": {"raw_dir": "..."}}
```

Direct from the editor's Python console:

```python
import player_movement.import_clips as m
m.run({"raw_dir": "C:/.../Content/Source/Mixamo/Raw"})
```

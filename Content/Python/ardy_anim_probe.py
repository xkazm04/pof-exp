"""Numerically probe the retargeted ARDY->Manny AnimSequences: sample pelvis/feet/head
world positions over time via the AnimPose API. The roll must show pelvis dip->recovery."""
import unreal

SRC = "/Game/Generated/Ardy/Manny"
CLIPS = ["combo1_Anim_Manny", "combo2_Anim_Manny", "combo3_Anim_Manny"]
BONES = ["pelvis", "foot_l", "foot_r", "head"]

for name in CLIPS:
    seq = unreal.EditorAssetLibrary.load_asset(f"{SRC}/{name}")
    if not seq:
        print(f"POF_PROBE_FAIL {name}")
        continue
    length = float(seq.get_play_length())
    times = [length * i / 7.0 for i in range(8)]
    rows = []
    for t in times:
        opts = unreal.AnimPoseEvaluationOptions()
        pose = unreal.AnimPoseExtensions.get_anim_pose_at_time(seq, t, opts)
        vals = []
        for b in BONES:
            tr = unreal.AnimPoseExtensions.get_bone_pose(pose, unreal.Name(b), unreal.AnimPoseSpaces.WORLD)
            loc = tr.translation
            vals.append((round(loc.x, 1), round(loc.y, 1), round(loc.z, 1)))
        rows.append((round(t, 2), vals))
    print(f"POF_PROBE {name} len={length:.2f}")
    for t, vals in rows:
        s = " ".join(f"{b}=({v[0]},{v[1]},{v[2]})" for b, v in zip(BONES, vals))
        print(f"POF_PROBE_T {t}: {s}")
print("POF_PROBE_DONE")

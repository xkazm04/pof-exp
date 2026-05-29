import sys
import types
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))


class _V:
    def __init__(self, x):
        self.x, self.y, self.z = x, 0.0, 0.0


class _T:
    def __init__(self, x):
        self.translation = _V(x)


fake = types.ModuleType("unreal")
_poses = {0.0: 0.0, 0.75: 30.0}  # Hips moves 0 -> 30 over time => animated


class _Anim:
    def get_play_length(self):
        return 1.5


fake.EditorAssetLibrary = type("E", (), {"load_asset": staticmethod(lambda p: _Anim())})()
fake.AnimationLibrary = type(
    "A", (), {"get_bone_pose_for_time": staticmethod(lambda a, b, t, extract: _T(_poses[t]))}
)
sys.modules["unreal"] = fake

from observation import evaluate_pose


def test_clip_static_detection_flags_motion():
    out = evaluate_pose.run({"mode": "clip", "asset_path": "/Game/X", "bones": ["Hips"]})
    assert out["kind"] == "pose"
    assert out["data"]["is_static"] is False
    assert out["data"]["max_bone_delta_over_time"] >= 29.0


def test_clip_static_detection_flags_refpose():
    fake.AnimationLibrary.get_bone_pose_for_time = staticmethod(lambda a, b, t, extract: _T(0.0))
    out = evaluate_pose.run({"mode": "clip", "asset_path": "/Game/X", "bones": ["Hips"]})
    assert out["data"]["is_static"] is True

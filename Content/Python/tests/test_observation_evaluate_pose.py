import math
import sys
import types
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))


class _Quat:
    def __init__(self, angle_deg):
        # rotation about Z by angle_deg, as a quaternion (x,y,z,w)
        half = math.radians(angle_deg) / 2.0
        self.x, self.y, self.z, self.w = 0.0, 0.0, math.sin(half), math.cos(half)


class _T:
    def __init__(self, angle_deg):
        self.rotation = _Quat(angle_deg)


fake = types.ModuleType("unreal")
# thigh_l rotates 0 -> 40deg across the cycle => animated
_angles = {0.0: 0.0, 0.375: 20.0, 0.75: 40.0, 1.125: 20.0}


class _Anim:
    def get_play_length(self):
        return 1.5


fake.EditorAssetLibrary = type("E", (), {"load_asset": staticmethod(lambda p: _Anim())})()
fake.AnimationLibrary = type(
    "A", (), {"get_bone_pose_for_time": staticmethod(lambda a, b, t, extract: _T(_angles[round(t, 3)]))}
)
sys.modules["unreal"] = fake

from observation import evaluate_pose


def test_clip_rotation_detection_flags_motion():
    out = evaluate_pose.run({"mode": "clip", "asset_path": "/Game/X", "bones": ["thigh_l"]})
    assert out["kind"] == "pose"
    assert out["data"]["is_static"] is False
    assert out["data"]["max_bone_rotation_deg"] >= 35.0


def test_clip_rotation_detection_flags_frozen():
    fake.AnimationLibrary.get_bone_pose_for_time = staticmethod(lambda a, b, t, extract: _T(0.0))
    out = evaluate_pose.run({"mode": "clip", "asset_path": "/Game/X", "bones": ["thigh_l"]})
    assert out["data"]["is_static"] is True
    assert out["data"]["max_bone_rotation_deg"] < 3.0

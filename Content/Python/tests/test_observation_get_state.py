import sys
import types
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

fake = types.ModuleType("unreal")


class _Anim:
    def get_class(self):
        return type("C", (), {"get_name": lambda s: "AnimSequence"})()

    def get_play_length(self):
        return 1.5


fake.EditorAssetLibrary = type("E", (), {"load_asset": staticmethod(lambda p: _Anim())})()
fake.AnimationLibrary = type(
    "A", (), {"get_num_keys": staticmethod(lambda a: 45), "get_num_frames": staticmethod(lambda a: 45)}
)
sys.modules["unreal"] = fake

from observation import get_state


def test_anim_sequence_state_reports_frames_and_length():
    out = get_state.run({"asset_path": "/Game/X"})
    assert out["kind"] == "state"
    assert out["data"]["class"] == "AnimSequence"
    assert out["data"]["num_keys"] == 45
    assert out["data"]["length"] == 1.5

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from observation import make_observation


def test_make_observation_shape():
    o = make_observation("state", {"sample_count": 11})
    assert o["kind"] == "state"
    assert o["data"] == {"sample_count": 11}
    assert "captured_at" in o and o["captured_at"]


def test_make_observation_carries_scenario():
    o = make_observation("pose", {"is_ref_pose": True}, scenario_id="walk-fwd")
    assert o["scenario_id"] == "walk-fwd"

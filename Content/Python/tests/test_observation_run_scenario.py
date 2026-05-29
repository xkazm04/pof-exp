import sys
import types
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

calls = {}
fake = types.ModuleType("unreal")


class _Struct:
    def __init__(self):
        self.action_path = ""
        self.value = None
        self.start_seconds = 0.0
        self.duration_seconds = 1.0

    def set_editor_property(self, k, v):
        setattr(self, k, v)


fake.PoFTimedInput = _Struct
fake.Vector2D = lambda x, y: (x, y)
fake.PoFScenarioRunner = type(
    "R", (), {"run_scenario": staticmethod(lambda m, inp, t, shot: calls.update({"map": m, "n": len(inp), "t": t, "shot": shot}) or True)}
)
sys.modules["unreal"] = fake

from observation import run_scenario


def test_builds_timed_inputs_and_calls_harness():
    out = run_scenario.run(
        {
            "map": "/Game/Maps/Test",
            "total_seconds": 1.5,
            "inputs": [{"action": "/Game/Input/Actions/IA_Move", "value": [0, 1], "start": 0, "duration": 1.5}],
        }
    )
    assert out["kind"] == "metric"
    assert out["data"]["started"] is True
    assert calls["map"] == "/Game/Maps/Test" and calls["n"] == 1

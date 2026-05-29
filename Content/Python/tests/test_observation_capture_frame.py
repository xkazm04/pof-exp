import sys
import types
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

fake = types.ModuleType("unreal")
fake.AutomationLibrary = type(
    "A", (), {"take_high_res_screenshot": staticmethod(lambda w, h, name, *a, **k: True)}
)
sys.modules["unreal"] = fake

from observation import capture_frame


def test_capture_returns_png_path():
    out = capture_frame.run({"out_name": "tpose_check", "width": 512, "height": 512})
    assert out["kind"] == "frame"
    assert out["data"]["png"].endswith("tpose_check.png")

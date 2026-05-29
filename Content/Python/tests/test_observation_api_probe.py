import sys
import types
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

fake = types.ModuleType("unreal")


class _Demo:
    @staticmethod
    def alpha(): ...
    @staticmethod
    def beta(): ...


fake.Demo = _Demo
fake.EditorAssetLibrary = type("E", (), {"load_asset": staticmethod(lambda p: None)})()
sys.modules["unreal"] = fake

from observation import api_probe


def test_class_methods_lists_public_callables():
    out = api_probe.run({"mode": "class_methods", "class_name": "Demo"})
    assert out["kind"] == "api"
    assert "alpha" in out["data"]["methods"] and "beta" in out["data"]["methods"]


def test_unknown_mode_reports_error():
    out = api_probe.run({"mode": "nope"})
    assert "error" in out["data"]

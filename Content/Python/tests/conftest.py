"""Pytest conftest — installs a minimal `unreal` stub before any test imports a pipeline module.

The real `unreal` module is only available inside the UE Python interpreter.
Tests stub the subset of the API our modules touch (asset lookup, registry,
factory + controller surface) so we can verify control-flow + idempotency on
a regular Python install (`pytest 8.x`).

Each test that needs custom behavior monkey-patches attributes on the stub.
"""

from __future__ import annotations

import sys
import types
from pathlib import Path

# Make the `player_movement` package importable from these tests
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))


def _build_unreal_stub():
    fake = types.ModuleType("unreal")

    class Vector:
        def __init__(self, x=0, y=0, z=0):
            self.x, self.y, self.z = x, y, z

    # Asset library — caller monkey-patches `_present` to control which paths exist
    class EditorAssetLibrary:
        _present: set[str] = set()
        _assets: dict[str, object] = {}

        @staticmethod
        def does_asset_exist(path):
            return path in EditorAssetLibrary._present

        @staticmethod
        def load_asset(path):
            # Mirror UE: load_asset returns None when the asset isn't present,
            # regardless of whether we have a stub object cached for it.
            if path not in EditorAssetLibrary._present:
                return None
            return EditorAssetLibrary._assets.get(path)

        @staticmethod
        def save_asset(path):
            return True

        @staticmethod
        def delete_asset(path):
            EditorAssetLibrary._present.discard(path)
            EditorAssetLibrary._assets.pop(path, None)
            return True

        @staticmethod
        def make_directory(path):
            return True

    class _AssetTools:
        @staticmethod
        def create_asset(name, package, cls, factory):
            obj = types.SimpleNamespace(_class=cls.__name__ if hasattr(cls, "__name__") else "asset",
                                         _name=name)
            EditorAssetLibrary._present.add(f"{package}/{name}")
            EditorAssetLibrary._assets[f"{package}/{name}"] = obj
            return obj

        @staticmethod
        def import_asset_tasks(tasks):
            # Mark each task's filename as having produced an asset in the destination
            for t in tasks:
                name = Path(t.filename).stem
                EditorAssetLibrary._present.add(f"{t.destination_path}/{name}")

    class AssetToolsHelpers:
        @staticmethod
        def get_asset_tools():
            return _AssetTools

    class AssetImportTask:
        def __init__(self):
            self.filename = ""
            self.destination_path = ""
            self.replace_existing = True
            self.automated = True
            self.save = True
            self.options = None

    class _AssetData:
        def __init__(self, object_path, asset_class="AnimSequence"):
            self.object_path = object_path
            self.asset_class = asset_class
            self.asset_class_path = types.SimpleNamespace(asset_name=asset_class)

    class _AssetRegistry:
        _path_to_assets: dict[str, list[_AssetData]] = {}

        def get_assets_by_path(self, path, recursive=True):
            return list(self._path_to_assets.get(path, []))

    class AssetRegistryHelpers:
        _registry = _AssetRegistry()

        @staticmethod
        def get_asset_registry():
            return AssetRegistryHelpers._registry

    class _IKRigController:
        @staticmethod
        def get_controller(rig):
            class _Inner:
                def set_skeleton(self, _): pass
                def add_retarget_chain(self, *_a, **_k): pass
            return _Inner()

    class _IKRetargeterController:
        last_inputs = None
        @staticmethod
        def get_controller(rtg):
            class _Inner:
                def set_ik_rig(self, *_a, **_k): pass
                def batch_retarget_animations(self, inputs):
                    _IKRetargeterController.last_inputs = inputs
                    out = []
                    for spec in inputs:
                        src = spec["source_animation"]
                        name = src.rsplit("/", 1)[-1]
                        dest = f"{spec['destination_folder']}/{name}_RT"
                        EditorAssetLibrary._present.add(dest)
                        out.append(types.SimpleNamespace(
                            get_path_name=lambda d=dest: d,
                        ))
                    return out
            return _Inner()

    class _BlendSpaceLibrary:
        _samples: list = []
        @staticmethod
        def get_sample_count(bs): return len(_BlendSpaceLibrary._samples)
        @staticmethod
        def add_sample(bs, anim, value): _BlendSpaceLibrary._samples.append((anim, value))
        @staticmethod
        def remove_sample(bs, index): _BlendSpaceLibrary._samples.pop(index)

    class _PoFAnimBPAuthoringLibrary:
        calls: list = []
        @staticmethod
        def create_anim_blueprint(skel, path, name):
            _PoFAnimBPAuthoringLibrary.calls.append(("create", name))
            obj = types.SimpleNamespace(_name=name, generated_class=types.SimpleNamespace())
            EditorAssetLibrary._present.add(f"{path}/{name}")
            EditorAssetLibrary._assets[f"{path}/{name}"] = obj
            return obj
        @staticmethod
        def add_state_machine(abp, sm): _PoFAnimBPAuthoringLibrary.calls.append(("sm", sm)); return True
        @staticmethod
        def add_blend_space_state(abp, sm, st, bs, s, d):
            _PoFAnimBPAuthoringLibrary.calls.append(("bss", st, s, d)); return True
        @staticmethod
        def add_default_slot(abp, n): _PoFAnimBPAuthoringLibrary.calls.append(("slot", n)); return True
        @staticmethod
        def connect_state_machine_to_output_pose(abp, sm, slot):
            _PoFAnimBPAuthoringLibrary.calls.append(("connect", sm, slot)); return True
        @staticmethod
        def compile_and_save(abp): _PoFAnimBPAuthoringLibrary.calls.append(("compile",)); return True

    # Wire stubs onto the fake module
    fake.Vector = Vector
    fake.EditorAssetLibrary = EditorAssetLibrary
    fake.AssetToolsHelpers = AssetToolsHelpers
    fake.AssetImportTask = AssetImportTask
    fake.AssetRegistryHelpers = AssetRegistryHelpers
    fake.IKRigController = _IKRigController
    fake.IKRetargeterController = _IKRetargeterController
    fake.BlendSpaceLibrary = _BlendSpaceLibrary
    fake.PoFAnimBPAuthoringLibrary = _PoFAnimBPAuthoringLibrary

    # Marker classes (constructors no-op; only used in isinstance / class refs)
    fake.IKRigDefinition = type("IKRigDefinition", (), {})
    fake.IKRetargeter = type("IKRetargeter", (), {})
    fake.AnimMontage = type("AnimMontage", (), {})
    fake.AnimSequence = type("AnimSequence", (), {})
    fake.AnimBlueprint = type("AnimBlueprint", (), {})
    fake.AnimNotify_DodgeWindow = type("AnimNotify_DodgeWindow", (), {})
    fake.StaticMeshActor = type("StaticMeshActor", (), {})
    fake.PlayerStart = type("PlayerStart", (), {})
    fake.RetargetSourceOrTarget = types.SimpleNamespace(SOURCE="SOURCE", TARGET="TARGET")
    fake.FBXImportType = types.SimpleNamespace(FBXIT_SKELETAL_MESH="skel", FBXIT_ANIMATION="anim")

    class _FbxImportUI:
        def __init__(self):
            self.import_animations = True
            self.import_mesh = False
            self.import_as_skeletal = False
        def set_editor_property(self, *_a, **_k): pass
    fake.FbxImportUI = _FbxImportUI

    class _IKRigDefinitionFactory:
        def set_editor_property(self, *_a, **_k): pass
    fake.IKRigDefinitionFactory = _IKRigDefinitionFactory

    class _AnimMontageFactory:
        def set_editor_property(self, *_a, **_k): pass
    fake.AnimMontageFactory = _AnimMontageFactory

    class _AnimationLibrary:
        @staticmethod
        def add_slot_animation_track(*_a, **_k): pass
        @staticmethod
        def add_animation_notify_event(*_a, **_k): pass
    fake.AnimationLibrary = _AnimationLibrary

    class _EditorLevelLibrary:
        @staticmethod
        def new_level(_): return True
        @staticmethod
        def spawn_actor_from_class(_cls, _v):
            return types.SimpleNamespace(
                static_mesh_component=types.SimpleNamespace(set_static_mesh=lambda *_: None),
                set_actor_scale3d=lambda *_: None,
            )
        @staticmethod
        def save_current_level(): return True
    fake.EditorLevelLibrary = _EditorLevelLibrary

    fake.get_default_object = lambda _cls: None
    fake.log_warning = lambda *_a, **_k: None

    return fake


# Install the stub once. Tests can monkey-patch attributes after import.
sys.modules["unreal"] = _build_unreal_stub()

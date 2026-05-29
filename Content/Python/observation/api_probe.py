"""ApiGroundingProbe (SP1) — query UE's real API/state before authoring.

Modes:
  class_methods : public callables on unreal.<class_name>
  asset_props   : public attributes of the loaded asset at asset_path
  assets_at     : asset package names under a content path
"""
import unreal

from observation import make_observation


def run(args):
    mode = args.get("mode")
    if mode == "class_methods":
        cls = getattr(unreal, args["class_name"], None)
        methods = sorted(m for m in dir(cls) if not m.startswith("_")) if cls else []
        return make_observation("api", {"class": args["class_name"], "methods": methods})
    if mode == "asset_props":
        asset = unreal.EditorAssetLibrary.load_asset(args["asset_path"])
        props = sorted(p for p in dir(asset) if not p.startswith("_")) if asset else []
        return make_observation("api", {"asset_path": args["asset_path"], "props": props})
    if mode == "assets_at":
        ar = unreal.AssetRegistryHelpers.get_asset_registry()
        f = unreal.ARFilter(package_paths=[args["path"]], recursive_paths=args.get("recursive", False))
        paths = sorted(str(a.package_name) for a in ar.get_assets(f))
        return make_observation("api", {"path": args["path"], "assets": paths})
    return make_observation("api", {"error": f"unknown mode: {mode}"})

"""Probe which animation-data-editing methods are exposed to Python (5.7) — to know whether
baking a root-motion track is possible from Python or needs C++."""
import unreal


def run(args):
    a = unreal.load_asset(args["asset"])
    kw = ("controller", "model", "track", "bone", "root", "key", "curve", "raw", "data", "modif")
    am = sorted(m for m in dir(a) if any(k in m.lower() for k in kw))
    al = sorted(m for m in dir(unreal.AnimationLibrary) if any(k in m.lower() for k in kw))
    have_ctrl_class = bool(getattr(unreal, "AnimDataController", None) or getattr(unreal, "AnimationDataController", None))
    return {"AnimSequence_methods": am, "AnimationLibrary_methods": al,
            "AnimDataController_class_exists": have_ctrl_class}

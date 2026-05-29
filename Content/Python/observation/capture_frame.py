"""CaptureFrame (SP1) — T4 perceptual read. Writes a PNG the agent then Reads."""
import os

import unreal

from observation import make_observation

SHOT_DIR = r"C:\Users\kazda\Documents\Unreal Projects\PoF\Saved\Observations"


def run(args):
    os.makedirs(SHOT_DIR, exist_ok=True)
    name = args.get("out_name", "frame")
    w, h = int(args.get("width", 512)), int(args.get("height", 512))
    png = os.path.join(SHOT_DIR, f"{name}.png")
    try:
        unreal.AutomationLibrary.take_high_res_screenshot(w, h, png)
        return make_observation("frame", {"png": png, "width": w, "height": h},
                                scenario_id=args.get("scenario_id"))
    except Exception as e:  # noqa: BLE001
        return make_observation("frame", {"error": str(e)})

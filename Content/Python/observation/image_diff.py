"""Golden-image diff (host-side, numpy+PIL) — the VISUAL-regression half of the gate.

Mirrors UE's Screenshot Comparison three-tier idea: per-pixel channel tolerance, a global
mean-diff budget, and a changed-pixel-fraction budget. Optional center-crop ROI focuses the
compare on the player and trims roaming background (e.g. the arena enemy) that would otherwise
read as a false regression. Writes a diff heatmap next to the incoming frame.

verdict = compare(golden_png, incoming_png, tol) -> dict(regressed: bool, ...metrics, diff_path)
First run (no golden) is handled by the caller, not here.
"""
import os

import numpy as np
from PIL import Image

# Default tolerances (0..1). A change must clear per_pixel on a channel to count as "changed";
# regression if changed-fraction OR global mean exceeds budget.
DEFAULT_TOL = {"per_pixel": 0.12, "changed_frac": 0.06, "global_mean": 0.04, "roi": 0.6}


def _load(path, size=None):
    im = Image.open(path).convert("RGB")
    if size is not None and im.size != size:
        im = im.resize(size, Image.BILINEAR)
    return np.asarray(im, dtype=np.float32) / 255.0


def _crop_roi(arr, roi):
    """Center-crop to a fraction `roi` of each dimension (roi=1.0 -> no crop)."""
    if roi >= 0.999:
        return arr
    h, w = arr.shape[:2]
    ch, cw = int(h * roi), int(w * roi)
    y0, x0 = (h - ch) // 2, (w - cw) // 2
    return arr[y0:y0 + ch, x0:x0 + cw]


def compare(golden_png, incoming_png, tol=None):
    t = {**DEFAULT_TOL, **(tol or {})}
    g = _load(golden_png)
    inc = _load(incoming_png, size=(g.shape[1], g.shape[0]))  # match golden size
    g, inc = _crop_roi(g, t["roi"]), _crop_roi(inc, t["roi"])

    chan_abs = np.abs(g - inc)                      # HxWx3
    per_pixel_max = chan_abs.max(axis=2)            # worst channel per pixel
    changed_mask = per_pixel_max > t["per_pixel"]
    changed_frac = float(changed_mask.mean())
    global_mean = float(chan_abs.mean())

    regressed = changed_frac > t["changed_frac"] or global_mean > t["global_mean"]

    # Heatmap: changed pixels in red over a dim grayscale of the incoming frame.
    base = (inc.mean(axis=2, keepdims=True) * 0.4).repeat(3, axis=2)
    base[changed_mask] = [1.0, 0.0, 0.0]
    diff_path = os.path.splitext(incoming_png)[0] + "_diff.png"
    Image.fromarray((base * 255).astype(np.uint8)).save(diff_path)

    return {
        "regressed": regressed,
        "changed_frac": round(changed_frac, 4),
        "global_mean": round(global_mean, 4),
        "changed_frac_budget": t["changed_frac"],
        "global_mean_budget": t["global_mean"],
        "diff_path": diff_path,
    }

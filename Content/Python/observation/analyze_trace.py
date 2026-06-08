"""Analyse a movement trace.json (written by UScenarioController) into a programmatic
overview: motion profile, capsule-vs-animation foot-slide, and a top-down trajectory plot.

HOST-side tool (plain stdlib) — run:  python analyze_trace.py <path-to-trace.json>

trace rows: {t,dt,x,y,vx,vy,speed,yaw,dCapX,dCapY,dRootX,dRootY,montage,montPos}
  dCap* = capsule displacement this tick (what actually moved)
  dRoot* = animation root-motion this tick (what the anim intended)
  foot-slide = capsule motion that the animation did NOT account for.
"""
import json
import math
import sys


def hyp(a, b):
    return math.hypot(a, b)


def main(path):
    with open(path, "r") as f:
        rows = json.load(f).get("trace", [])
    if not rows:
        print("(empty trace)")
        return

    cum_cap = cum_root = cum_slide = 0.0
    peak_speed = 0.0
    peak_t = 0.0
    phases = []  # (montage, t_start, t_end, cap, root)
    cur = None
    print(f"{'t':>5} {'speed':>7} {'cumCap':>8} {'cumRoot':>8} {'slide':>7} {'montage':>12}")
    n = len(rows)
    step = max(1, n // 24)  # decimate to ~24 printed lines
    for i, r in enumerate(rows):
        dc = hyp(r.get("dCapX", 0), r.get("dCapY", 0))
        dr = hyp(r.get("dRootX", 0), r.get("dRootY", 0))
        cum_cap += dc
        cum_root += dr
        cum_slide += abs(dc - dr)
        sp = r.get("speed", 0)
        if sp > peak_speed:
            peak_speed, peak_t = sp, r.get("t", 0)
        m = r.get("montage", "") or "-"
        if cur is None or cur[0] != m:
            if cur:
                phases.append(cur)
            cur = [m, r.get("t", 0), r.get("t", 0), 0.0, 0.0]
        cur[2] = r.get("t", 0); cur[3] += dc; cur[4] += dr
        if i % step == 0 or i == n - 1:
            print(f"{r.get('t',0):5.2f} {sp:7.0f} {cum_cap:8.0f} {cum_root:8.0f} {cum_slide:7.0f} {m:>12}")
    if cur:
        phases.append(cur)

    print("\n=== SUMMARY ===")
    print(f"duration            {rows[-1].get('t',0):.2f}s ({n} ticks)")
    print(f"capsule distance    {cum_cap:.0f}u   (what the character actually traveled)")
    print(f"anim root-motion    {cum_root:.0f}u   (what the animation would move it)")
    print(f"FOOT-SLIDE (desync) {cum_slide:.0f}u   (capsule motion the animation didn't account for)")
    print(f"peak speed          {peak_speed:.0f} u/s at t={peak_t:.2f}s")

    print("\n=== PHASES (by active montage) ===")
    for m, t0, t1, cap, root in phases:
        print(f"  {m:>14}  t={t0:.2f}-{t1:.2f}s  capsule={cap:.0f}u  root={root:.0f}u")

    # Top-down trajectory plot (X right, Y up), capsule path '*', start 'S', end 'E'.
    xs = [r.get("x", 0) for r in rows]
    ys = [r.get("y", 0) for r in rows]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    W, H = 56, 18
    span_x = max(x1 - x0, 1.0)
    span_y = max(y1 - y0, 1.0)
    grid = [[" "] * W for _ in range(H)]

    def plot(x, y, ch):
        cx = int((x - x0) / span_x * (W - 1))
        cy = int((y - y0) / span_y * (H - 1))
        grid[H - 1 - cy][cx] = ch

    for r in rows:
        plot(r.get("x", 0), r.get("y", 0), "*")
    plot(xs[0], ys[0], "S")
    plot(xs[-1], ys[-1], "E")
    print(f"\n=== TOP-DOWN TRAJECTORY (X {x0:.0f}..{x1:.0f}, Y {y0:.0f}..{y1:.0f}; S=start E=end) ===")
    for row in grid:
        print("".join(row))


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "trace.json")

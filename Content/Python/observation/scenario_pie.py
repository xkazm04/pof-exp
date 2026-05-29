"""Drive the runtime UScenarioController via NATURAL PIE (faithful game loop).

The editor must be launched with -PoFScenario=<inbox.json>. Per run: rewrite the inbox
file, call start(map) (PIE starts + ticks on the editor's own loop — no manual tick),
poll for <out_dir>/DONE, read observations.json + frame_NN.png, then stop().
"""
import unreal


def start(args):
    return {"started": bool(unreal.PoFScenarioRunner.start_pie(args["map"]))}


def stop(args):
    unreal.PoFScenarioRunner.stop_scenario()
    return {"stopped": True}

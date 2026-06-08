"""Open a level in the EDITOR (not PIE) so a human can play-test it.

Used to restore a lit, visible level after make_test_map switched the editor to the
sparse (dark) TestHarness. Call via /pof/python/run {module:"observation.open_level",
function:"run", args:{map:"/Game/Maps/VerticalSlice"}}.
"""
import unreal


def run(args):
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    les.load_level(args["map"])
    return {"opened": args["map"]}

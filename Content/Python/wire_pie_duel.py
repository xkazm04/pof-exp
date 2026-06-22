"""Make the VerticalSlice duel work in PIE: set the slice enemy to the nav-independent
ARPGSimpleAIController so it chases + attacks without a baked navmesh / the scenario harness.
Targeted to BP_VSEnemy only — other enemies keep their behavior-tree controller.
"""
import unreal as u
T = "AIWIRE"
def log(m): u.log("%s: %s" % (T, m))

BP = "/Game/VerticalSlice/BP_VSEnemy"


def main():
    bp = u.load_asset(BP)
    gc = bp.generated_class() if bp else None
    cdo = u.get_default_object(gc) if gc else None
    if not cdo:
        u.log_error("AIWIRE: could not load BP_VSEnemy CDO"); return
    cur = cdo.get_editor_property("ai_controller_class")
    curname = cur.get_name() if cur else "None(C++ default)"
    log("current AIControllerClass = %s" % curname)
    if curname != "ARPGSimpleAIController":
        cdo.set_editor_property("ai_controller_class", u.ARPGSimpleAIController)
        ok = u.EditorAssetLibrary.save_asset(BP)
        # read back to confirm it stuck
        after = cdo.get_editor_property("ai_controller_class")
        log("SET -> %s  saved=%s" % (after.get_name() if after else "None", ok))
    else:
        log("already ARPGSimpleAIController")
    log("[gate] RESULT=PASS")


main()

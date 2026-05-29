"""Step 07 acceptance — verify the PoFEditor AnimBP authoring library is loaded.

After PoFEditor is rebuilt + the editor restarted, `unreal.PoFAnimBPAuthoringLibrary`
must resolve before step 08 (build_anim_bp) can author ABP_VSPlayer.

Returns {"ok": bool, "issues": [...]}.
"""

import unreal


def run(args):
    issues = []
    lib = getattr(unreal, "PoFAnimBPAuthoringLibrary", None)
    if lib is None:
        issues.append(
            "unreal.PoFAnimBPAuthoringLibrary not available — rebuild PoFEditor + "
            "restart the editor (pipeline step 07)"
        )
    else:
        # Confirm the six authoring entrypoints are exposed as script methods.
        for fn in (
            "create_anim_blueprint",
            "add_state_machine",
            "add_blend_space_state",
            "add_default_slot",
            "connect_state_machine_to_output_pose",
            "compile_and_save",
        ):
            if not hasattr(lib, fn):
                issues.append(f"PoFAnimBPAuthoringLibrary missing method: {fn}")

    return {"ok": len(issues) == 0, "issues": issues}

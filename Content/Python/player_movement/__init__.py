"""PoF player movement pipeline modules (steps 3–9 of the player-movement catalog row).

Each module exposes `run(args: dict) -> dict`. Modules are idempotent:
re-running with the same inputs is a no-op (existing artifacts are skipped).

Standard return envelope::

    {
        "created": [str, ...],   # new artifacts produced this run
        "updated": [str, ...],   # existing artifacts that were modified
        "skipped": [str, ...],   # artifacts already in the target state
        "failed":  [str, ...],   # human-readable failure messages
    }

Modules:
    import_clips        — batch FBX import to /Game/Mixamo/Raw/
    build_ik_rigs       — IK_Mixamo + IK_Manny + RTG_MixamoToManny
    retarget            — batch retarget to /Game/Mixamo/Retargeted/SKM_Manny/
    build_blend_space   — program BS_Locomotion sample grid (8-way + back)
    build_anim_bp       — author ABP_VSPlayer via PoFAnimBPAuthoringLibrary
    build_montage       — build AM_Roll from Forward_Roll_RT
    build_test_level    — create TestLevel_PlayerMovement.umap for the L4 gate
    verify_mesh         — step 01 acceptance: BP_VSPlayer mesh + IMC bindings
"""

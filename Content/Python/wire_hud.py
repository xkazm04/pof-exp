"""
wire_hud.py
-----------
Sets HUDClass = AVSHUD on the BP_VSGameMode Blueprint and saves it.
Safe to re-run (idempotent).
"""
import unreal

GAMEMODE_ASSET = '/Game/VerticalSlice/BP_VSGameMode'
HUD_CLASS_PATH  = '/Script/PoF.VSHUD'

try:
    unreal.log('=== wire_hud: start ===')

    # 1. Load the Blueprint asset
    unreal.log(f'Loading Blueprint asset: {GAMEMODE_ASSET}')
    bp = unreal.EditorAssetLibrary.load_asset(GAMEMODE_ASSET)
    if bp is None:
        raise RuntimeError(f'Could not load asset: {GAMEMODE_ASSET}')
    unreal.log(f'Loaded: {bp}')

    # 2. Resolve the AVSHUD class
    unreal.log(f'Resolving HUD class from: {HUD_CLASS_PATH}')
    hud_class = unreal.load_class(None, HUD_CLASS_PATH)
    if hud_class is None:
        # Fallback: try direct Python binding exposed by the engine
        unreal.log_warning(
            f'load_class returned None for {HUD_CLASS_PATH}, trying unreal.VSHUD binding'
        )
        hud_class = getattr(unreal, 'VSHUD', None)
    if hud_class is None:
        raise RuntimeError(
            f'Failed to resolve AVSHUD class via {HUD_CLASS_PATH} or unreal.VSHUD'
        )
    unreal.log(f'Resolved HUD class: {hud_class}')

    # 3. Get the generated class and its CDO
    unreal.log('Fetching generated class CDO ...')
    gen_class = bp.generated_class()
    if gen_class is None:
        raise RuntimeError('Blueprint has no generated_class — has it been compiled?')
    cdo = unreal.get_default_object(gen_class)
    if cdo is None:
        raise RuntimeError('Could not get CDO from generated_class')
    unreal.log(f'CDO: {cdo}')

    # 4. Read current value for idempotency log
    current = cdo.get_editor_property('HUDClass')
    unreal.log(f'Current HUDClass (before): {current}')

    # 5. Set HUDClass
    cdo.set_editor_property('HUDClass', hud_class)
    unreal.log('set_editor_property HUDClass done')

    # 6. Read back to confirm
    after = cdo.get_editor_property('HUDClass')
    unreal.log(f'HUDClass (after, read-back): {after}')
    if after is None:
        raise RuntimeError('HUDClass read-back is None — set may have failed')

    # 7. Save the Blueprint asset
    unreal.log(f'Saving asset: {GAMEMODE_ASSET}')
    saved = unreal.EditorAssetLibrary.save_asset(GAMEMODE_ASSET)
    unreal.log(f'save_asset returned: {saved}')

    unreal.log('=== HUD wiring COMPLETE ===')

except Exception as e:
    unreal.log_error(f'wire_hud FAILED: {e}')
    raise

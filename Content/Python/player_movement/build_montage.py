"""Step 09 — Build AM_Roll montage from the retargeted Forward_Roll clip (UE 5.7).

The montage's slot animation is populated by setting AnimMontageFactory.source_animation
(there is no Python API to add a slot track after creation). An AnimNotifyState_DodgeIFrame
is added on a "DodgeWindow" notify track for the iframe window.
"""

import unreal


PACKAGE = "/Game/Characters/Player/Animations"
ASSET_NAME = "AM_Roll"
SRC_PATH = "/Game/Mixamo/Retargeted/SKM_Manny/Forward_Roll_RT"

DODGE_WINDOW_START = 2.0 / 30.0   # frame 2
DODGE_WINDOW_DURATION = 4.0 / 30.0  # ~4 frames of iframes


def run(args):
    result = {"created": [], "skipped": [], "failed": []}

    src = unreal.EditorAssetLibrary.load_asset(SRC_PATH)
    if not src:
        result["failed"].append(f"missing source clip: {SRC_PATH}; run step 05 first")
        return result

    target_path = f"{PACKAGE}/{ASSET_NAME}"
    # Recreate cleanly — montage slot content can only be set at creation time via
    # the factory's source_animation, so a previously-empty AM_Roll must be replaced.
    if unreal.EditorAssetLibrary.does_asset_exist(target_path):
        unreal.EditorAssetLibrary.delete_asset(target_path)

    try:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        factory = unreal.AnimMontageFactory()
        factory.set_editor_property("source_animation", src)
        try:
            factory.set_editor_property("target_skeleton", src.get_skeleton())
        except Exception:
            pass

        montage = tools.create_asset(ASSET_NAME, PACKAGE, unreal.AnimMontage, factory)
        if not montage:
            result["failed"].append(f"create_asset failed for {ASSET_NAME}")
            return result

        # iframe NotifyState on a dedicated track
        notify_state = getattr(unreal, "AnimNotifyState_DodgeIFrame", None)
        if notify_state:
            try:
                unreal.AnimationLibrary.add_animation_notify_track(montage, "DodgeWindow")
                unreal.AnimationLibrary.add_animation_notify_state_event(
                    montage, "DodgeWindow", DODGE_WINDOW_START, DODGE_WINDOW_DURATION, notify_state
                )
            except Exception as e:  # noqa: BLE001
                # Non-fatal: the montage plays the roll without iframes.
                result["failed"].append(f"add iframe notify: {e}")

        unreal.EditorAssetLibrary.save_asset(target_path)
        result["created"].append(ASSET_NAME)
    except Exception as e:  # noqa: BLE001
        result["failed"].append(str(e))

    return result

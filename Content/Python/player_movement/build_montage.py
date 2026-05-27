"""Step 09 — Build AM_Roll montage from the retargeted Forward_Roll clip.

Adds an `AnimNotify_DodgeWindow` notify at frame 2 (~0.067s @ 30fps) which the
C++ side (`AARPGPlayerCharacter::SetRollIFrameActive`) reads to enable iframes.
"""

import unreal


PACKAGE = "/Game/Characters/Player/Animations"
ASSET_NAME = "AM_Roll"
SRC_PATH = "/Game/Mixamo/Retargeted/SKM_Manny/Forward_Roll_RT"

DODGE_WINDOW_TIME = 2.0 / 30.0  # frame 2 at 30 fps


def run(args):
    result = {"created": [], "skipped": [], "failed": []}

    src = unreal.EditorAssetLibrary.load_asset(SRC_PATH)
    if not src:
        result["failed"].append(f"missing source clip: {SRC_PATH}; run step 05 first")
        return result

    target_path = f"{PACKAGE}/{ASSET_NAME}"
    if unreal.EditorAssetLibrary.does_asset_exist(target_path):
        result["skipped"].append(ASSET_NAME)
        return result

    try:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        # AnimMontageFactory may not be available in every UE version; fall back to
        # AnimationAssetFactory or direct creation.
        factory = None
        try:
            factory = unreal.AnimMontageFactory()
            if hasattr(factory, "set_editor_property"):
                try:
                    factory.set_editor_property("preview_anim_sequence", src)
                except Exception:
                    pass
                try:
                    factory.set_editor_property("target_skeleton", src.get_skeleton())
                except Exception:
                    pass
        except Exception:
            factory = None

        montage = tools.create_asset(ASSET_NAME, PACKAGE, unreal.AnimMontage, factory)
        if not montage:
            result["failed"].append(f"create_asset failed for {ASSET_NAME}")
            return result

        # Slot anim track: DefaultGroup.DefaultSlot referencing src
        try:
            unreal.AnimationLibrary.add_slot_animation_track(montage, "DefaultSlot", src)
        except Exception as e:
            result["failed"].append(f"add_slot_animation_track: {e}")

        # iframe notify at frame 2
        try:
            notify_class = getattr(unreal, "AnimNotify_DodgeWindow", None)
            if notify_class:
                unreal.AnimationLibrary.add_animation_notify_event(
                    montage, DODGE_WINDOW_TIME, 0.0, notify_class, "DodgeWindow"
                )
        except Exception as e:
            # Non-fatal: the montage is usable without iframes, just won't have invuln.
            result["failed"].append(f"add iframe notify: {e}")

        unreal.EditorAssetLibrary.save_asset(target_path)
        result["created"].append(ASSET_NAME)
    except Exception as e:
        result["failed"].append(str(e))

    return result

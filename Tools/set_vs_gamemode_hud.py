"""
set_vs_gamemode_hud.py — point the vertical-slice game mode at the reparented
real HUD (AARPGHUD) instead of AVSHUD.

Run headlessly AFTER PoFEditor is rebuilt with the reparented widget family:

  UnrealEditor-Cmd.exe "<PoF.uproject>" \
      -ExecutePythonScript="<this file>" -unattended -nopause -nosplash -stdout

Idempotent: a no-op if BP_VSGameMode.HUDClass is already AARPGHUD.

Folder-04 HUD/UI §5 ("resolve the UARPGHUDWidget family", reparent-to-code path).
See docs/superpowers/plans/2026-05-24-hud-ui-04-game-reparent.md in the PoF app repo.
"""
import unreal

GAMEMODE_PATH = "/Game/VerticalSlice/BP_VSGameMode"
# HUDClass is a non-bool class pointer, so no leading-'b' mangling applies; UE Python
# usually exposes it snake_cased, but try the PascalCase fallback too.
PROP_CANDIDATES = ("hud_class", "HUDClass")


def _log(msg):
    unreal.log("[set_vs_hud] " + str(msg))


def _err(msg):
    unreal.log_error("[set_vs_hud] " + str(msg))


def _target_hud_class():
    # AARPGHUD is exposed to Python as unreal.ARPGHUD (the leading 'A' prefix is dropped).
    # Return the UClass (static_class()), not the Python type wrapper: a TSubclassOf
    # property reads back as an unreal.Class, and `unreal.ARPGHUD == <Class>` is False.
    cls = getattr(unreal, "ARPGHUD", None)
    return cls.static_class() if cls is not None else None


def main():
    target = _target_hud_class()
    if target is None:
        _err("unreal.ARPGHUD not found — is the PoF module compiled with AARPGHUD?")
        return False

    if not unreal.EditorAssetLibrary.does_asset_exist(GAMEMODE_PATH):
        _err("asset not found: %s" % GAMEMODE_PATH)
        return False

    bp = unreal.EditorAssetLibrary.load_asset(GAMEMODE_PATH)
    if bp is None:
        _err("failed to load: %s" % GAMEMODE_PATH)
        return False

    gen_class = bp.generated_class()
    if gen_class is None:
        _err("BP has no generated class (needs compile?)")
        return False

    cdo = unreal.get_default_object(gen_class)

    prop = None
    current = None
    for name in PROP_CANDIDATES:
        try:
            current = cdo.get_editor_property(name)
            prop = name
            break
        except Exception:
            continue
    if prop is None:
        _err("could not read HUDClass under any of %s" % (PROP_CANDIDATES,))
        return False

    if current == target:
        _log("HUDClass already AARPGHUD — no change (idempotent).")
        return True

    try:
        cdo.set_editor_property(prop, target)
    except Exception as exc:
        _err("set_editor_property(%s) failed: %s" % (prop, exc))
        return False

    saved = unreal.EditorAssetLibrary.save_asset(GAMEMODE_PATH, only_if_is_dirty=False)
    verify = cdo.get_editor_property(prop)
    _log("HUDClass: %s -> %s ; saved=%s" % (current, verify, saved))
    return verify == target


if main():
    unreal.log("[set_vs_hud] OK")
else:
    unreal.log_error("[set_vs_hud] FAILED")

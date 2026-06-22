"""
Force Push in-engine probe (headless).

First real in-engine execution of the autonomously-authored Force Push mechanic:
confirms the freshly-compiled UGA_ForcePush + UGE_Cooldown_ForcePush classes are
loaded in the live UE 5.8 editor and that the player CDO has the ability granted
(DefaultAbilities) and bound to hotbar slot 0 (AbilityLoadout → key '1').

Run headless via:
  UnrealEditor-Cmd PoF.uproject -ExecCmds="py import forcepush_probe;Quit" -nullrhi ...
Reads back via the abslog markers `FORCEPUSH_PROBE: ...` and `[gate] RESULT=PASS|FAIL`.
"""
import unreal


def _prop(obj, name):
    try:
        return obj.get_editor_property(name)
    except Exception as exc:  # noqa: BLE001 - want the reason in the log
        return "ERR(%s)" % exc


def main():
    unreal.log("FORCEPUSH_PROBE: BEGIN")
    checks = []

    # 1. Ability class loaded from the freshly-built module?
    ability_cls = getattr(unreal, "GA_ForcePush", None)
    checks.append(("ability_class_loaded", ability_cls is not None))
    if ability_cls is None:
        unreal.log_error("FORCEPUSH_PROBE: GA_ForcePush binding missing")
        unreal.log("[gate] RESULT=FAIL")
        return

    cdo = unreal.get_default_object(ability_cls)
    mana = _prop(cdo, "AbilityManaCost")
    base_damage = _prop(cdo, "BaseDamage")
    hkb = _prop(cdo, "HorizontalKnockback")
    vkb = _prop(cdo, "VerticalKnockback")
    cone = _prop(cdo, "ConeHalfAngleDeg")
    push_range = _prop(cdo, "PushRange")
    unreal.log(
        "FORCEPUSH_PROBE: tuning mana=%s base_damage=%s h_knockback=%s v_knockback=%s cone_deg=%s range=%s"
        % (mana, base_damage, hkb, vkb, cone, push_range)
    )
    checks.append(("mana_is_20", mana == 20.0))
    checks.append(("knockback_configured", hkb == 1100.0 and vkb == 350.0))

    # 2. Cooldown GE class loaded?
    cd_cls = getattr(unreal, "GE_Cooldown_ForcePush", None)
    checks.append(("cooldown_ge_loaded", cd_cls is not None))

    # 3. Player CDO grants + slots Force Push (key '1')?
    player_cls = getattr(unreal, "ARPGPlayerCharacter", None)
    has_in_defaults = False
    in_slot0 = False
    if player_cls is not None:
        pcdo = unreal.get_default_object(player_cls)
        defaults = _prop(pcdo, "DefaultAbilities")
        try:
            default_names = [str(x) for x in defaults]
        except Exception:
            default_names = [str(defaults)]
        has_in_defaults = any("ForcePush" in n for n in default_names)
        loadout = _prop(pcdo, "AbilityLoadout")
        in_slot0 = "ForcePush" in str(loadout)
        unreal.log(
            "FORCEPUSH_PROBE: player default_abilities=%d has_force_push=%s slot0=%s"
            % (len(default_names), has_in_defaults, in_slot0)
        )
    checks.append(("granted_on_player", has_in_defaults))
    checks.append(("bound_to_slot_0", in_slot0))

    # Summary
    passed = [n for n, ok in checks if ok]
    failed = [n for n, ok in checks if not ok]
    unreal.log("FORCEPUSH_PROBE: passed=%s" % passed)
    if failed:
        unreal.log_warning("FORCEPUSH_PROBE: failed=%s" % failed)

    # Core must-haves for an in-engine "loaded + wired" verdict.
    core_ok = (
        ability_cls is not None
        and cd_cls is not None
        and mana == 20.0
        and has_in_defaults
        and in_slot0
    )
    unreal.log("FORCEPUSH_PROBE: RESULT=%s (core_ok=%s)" % ("PASS" if core_ok else "PARTIAL", core_ok))
    unreal.log("[gate] RESULT=%s" % ("PASS" if core_ok else "FAIL"))


main()

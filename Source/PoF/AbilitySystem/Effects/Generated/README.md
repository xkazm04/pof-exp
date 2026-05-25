# Generated GameplayEffects (PoF "Generate C++" — ECW B3a)

**AUTO-GENERATED — do not hand-edit.** These `UGameplayEffect` subclasses are
emitted from a catalog ability's authored Effect Mapping (the app's enriched
ability spec). Regenerate from the ability's **Logic → Effect Mapping → Generate C++**
button; edits here will be overwritten.

This folder is **additive**: it never modifies hand-written `GE_*` classes.

## Source ability

- **Fireball** (`Ability.Fire.Fireball`, Offensive / Fire / advanced)

## Files

### GameplayEffects (`Effects/Generated/`)

| File | Effect | Duration | Modifiers | Granted tags |
|------|--------|----------|-----------|--------------|
| `GE_Gen_Fireball_FireImpact.{h,cpp}` | Fire Impact | Instant | `Health += -40` (Additive) | — |
| `GE_Gen_Fireball_Burning.{h,cpp}` | Burning | HasDuration 3.0s, period 1.0s | `Health += -5` per tick (Additive) | `State.Burning` |

### Wiring ability (`Abilities/Generated/`) — ECW B3b

| File | Class | Mana | Activation rules | Applies |
|------|-------|------|------------------|---------|
| `GA_Gen_Fireball.{h,cpp}` | `UGA_Gen_Fireball : UARPGGameplayAbility` | 20 | blocked while `State.Dead` / `State.Stunned` | both GEs above (to target) |

**Bespoke step required:** `GA_Gen_Fireball::ActivateAbility` applies the damaging effects to a placeholder target (the owner's own ASC). Replace it with real target acquisition (projectile / trace / lock-on) — that cannot be generated from the spec alone. Cooldown GE is also a TODO (out of B3b scope).

## Attribute mapping

All modifier attributes resolved against `UARPGAttributeSet`:

- `Health` → `UARPGAttributeSet::GetHealthAttribute()` ✓

## Tag delta — tags referenced that are NOT declared in `ARPGGameplayTags.h`

- **`State.Burning`** — granted by `GE_Gen_Fireball_Burning`. Not declared natively,
  so the grant is currently skipped at runtime (the class still compiles). To activate it:
  1. Add `POF_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Burning);` to `ARPGGameplayTags.h`;
  2. Add `UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Burning, "State.Burning", "On fire — taking damage over time");` to `ARPGGameplayTags.cpp`;
  3. Reference `ARPGGameplayTags::State_Burning` in the effect's `FInheritedTagContainer` (see `GE_Stun.cpp` for the pattern).

(Activation tag rules — `blocks`/`cancels`/`requires` — are deferred to B3b; they
belong on the ability's `ActivationBlockedTags`/`ActivationRequiredTags`, not the effect.)

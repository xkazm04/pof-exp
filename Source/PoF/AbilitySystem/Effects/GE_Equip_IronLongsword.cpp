#include "AbilitySystem/Effects/GE_Equip_IronLongsword.h"
#include "AbilitySystem/ARPGAttributeSet.h"

UGE_Equip_IronLongsword::UGE_Equip_IronLongsword()
{
	// Equip bonus persists while the weapon is equipped; the inventory component
	// removes it on unequip via the stored FActiveGameplayEffectHandle.
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// +15 AttackPower — canonical average of the Iron Longsword's 12-18 damage
	// (catalog entity item-1, numericValue 15). Mirrors the GE_Buff_WarCry pattern.
	FGameplayModifierInfo AttackPowerMod;
	AttackPowerMod.Attribute = UARPGAttributeSet::GetAttackPowerAttribute();
	AttackPowerMod.ModifierOp = EGameplayModOp::Additive;

	FScalableFloat AttackPowerValue;
	AttackPowerValue.Value = 15.f;
	AttackPowerMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttackPowerValue);

	Modifiers.Add(AttackPowerMod);
}

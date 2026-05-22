#include "AbilitySystem/Effects/GE_Heal.h"
#include "AbilitySystem/ARPGAttributeSet.h"

UGE_Heal::UGE_Heal()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Write to IncomingHeal meta attribute so PostGameplayEffectExecute
	// handles clamping, damage number broadcast, and GameplayCue.Heal.
	FGameplayModifierInfo HealMod;
	HealMod.Attribute = UARPGAttributeSet::GetIncomingHealAttribute();
	HealMod.ModifierOp = EGameplayModOp::Additive;

	FScalableFloat HealValue;
	HealValue.Value = 25.f;
	HealMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealValue);

	Modifiers.Add(HealMod);
}

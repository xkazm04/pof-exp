#include "AbilitySystem/Effects/GE_Buff_Strength.h"
#include "AbilitySystem/ARPGAttributeSet.h"

UGE_Buff_Strength::UGE_Buff_Strength()
{
	// Duration-based: lasts 10 seconds, then auto-removes
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FScalableFloat DurationValue;
	DurationValue.Value = 10.f;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationValue);

	// Modifier: add 15 Strength while active
	FGameplayModifierInfo StrengthMod;
	StrengthMod.Attribute = UARPGAttributeSet::GetStrengthAttribute();
	StrengthMod.ModifierOp = EGameplayModOp::Additive;

	FScalableFloat StrengthValue;
	StrengthValue.Value = 15.f;
	StrengthMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(StrengthValue);

	Modifiers.Add(StrengthMod);
}

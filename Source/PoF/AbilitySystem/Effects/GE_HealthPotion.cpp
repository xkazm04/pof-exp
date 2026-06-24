#include "AbilitySystem/Effects/GE_HealthPotion.h"
#include "AbilitySystem/ARPGAttributeSet.h"

UGE_HealthPotion::UGE_HealthPotion()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Write to IncomingHeal meta attribute so PostGameplayEffectExecute
	// handles clamping to MaxHealth, heal-number broadcast, and GameplayCue.Heal.
	FGameplayModifierInfo HealMod;
	HealMod.Attribute = UARPGAttributeSet::GetIncomingHealAttribute();
	HealMod.ModifierOp = EGameplayModOp::Additive;

	FScalableFloat HealValue;
	HealValue.Value = 50.f;
	HealMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealValue);

	Modifiers.Add(HealMod);
}

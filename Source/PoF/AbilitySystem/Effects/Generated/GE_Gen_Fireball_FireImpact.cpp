#include "AbilitySystem/Effects/Generated/GE_Gen_Fireball_FireImpact.h"
#include "AbilitySystem/ARPGAttributeSet.h"

UGE_Gen_Fireball_FireImpact::UGE_Gen_Fireball_FireImpact()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Modifier: Health += -35 (canonical Fireball damage from the spellbook entity)
	FGameplayModifierInfo HealthMod;
	HealthMod.Attribute = UARPGAttributeSet::GetHealthAttribute();
	HealthMod.ModifierOp = EGameplayModOp::Additive;

	FScalableFloat HealthValue;
	HealthValue.Value = -35.f;
	HealthMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealthValue);

	Modifiers.Add(HealthMod);
}

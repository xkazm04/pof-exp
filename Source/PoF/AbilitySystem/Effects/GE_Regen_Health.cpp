#include "AbilitySystem/Effects/GE_Regen_Health.h"
#include "AbilitySystem/ARPGAttributeSet.h"

UGE_Regen_Health::UGE_Regen_Health()
{
	// Infinite duration — runs until explicitly removed
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// Period: tick every 2 seconds, execute (not aggregate) each tick
	Period.Value = 2.f;
	bExecutePeriodicEffectOnApplication = false;

	// Modifier: add 5 Health per tick
	FGameplayModifierInfo RegenMod;
	RegenMod.Attribute = UARPGAttributeSet::GetHealthAttribute();
	RegenMod.ModifierOp = EGameplayModOp::Additive;

	FScalableFloat RegenValue;
	RegenValue.Value = 5.f;
	RegenMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(RegenValue);

	Modifiers.Add(RegenMod);
}

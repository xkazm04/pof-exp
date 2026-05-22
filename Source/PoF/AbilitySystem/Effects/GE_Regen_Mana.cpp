#include "AbilitySystem/Effects/GE_Regen_Mana.h"
#include "AbilitySystem/ARPGAttributeSet.h"

UGE_Regen_Mana::UGE_Regen_Mana()
{
	// Infinite duration -- runs until explicitly removed
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// Period: tick every 2 seconds
	Period.Value = 2.f;
	bExecutePeriodicEffectOnApplication = false;

	// Modifier: add 3 Mana per tick (clamped to MaxMana by AttributeSet)
	FGameplayModifierInfo RegenMod;
	RegenMod.Attribute = UARPGAttributeSet::GetManaAttribute();
	RegenMod.ModifierOp = EGameplayModOp::Additive;

	FScalableFloat RegenValue;
	RegenValue.Value = 3.f;
	RegenMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(RegenValue);

	Modifiers.Add(RegenMod);
}

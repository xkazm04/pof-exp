#include "AbilitySystem/Effects/GE_ManaCost.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"

UGE_ManaCost::UGE_ManaCost()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Deduct Mana — caller must set a negative SetByCaller value
	// (e.g., SetSetByCallerMagnitude(Data_ManaCost, -20.f) to deduct 20 Mana).
	FGameplayModifierInfo ManaMod;
	ManaMod.Attribute = UARPGAttributeSet::GetManaAttribute();
	ManaMod.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = ARPGGameplayTags::Data_ManaCost;

	ManaMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(ManaMod);
}

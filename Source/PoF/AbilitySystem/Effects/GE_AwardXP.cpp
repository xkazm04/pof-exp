#include "AbilitySystem/Effects/GE_AwardXP.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"

UGE_AwardXP::UGE_AwardXP()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Add IncomingXP via SetByCaller (Additive) — consumed by PostGameplayEffectExecute
	FGameplayModifierInfo XPMod;
	XPMod.Attribute = UARPGAttributeSet::GetIncomingXPAttribute();
	XPMod.ModifierOp = EGameplayModOp::Additive;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = ARPGGameplayTags::Data_XP_Amount;

	XPMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(XPMod);
}

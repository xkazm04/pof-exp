#include "AbilitySystem/Effects/GE_Buff_WarCry.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Buff_WarCry::UGE_Buff_WarCry()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FScalableFloat DurationValue;
	DurationValue.Value = 15.f;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationValue);

	// +20 AttackPower
	FGameplayModifierInfo APMod;
	APMod.Attribute = UARPGAttributeSet::GetAttackPowerAttribute();
	APMod.ModifierOp = EGameplayModOp::Additive;
	FScalableFloat APValue;
	APValue.Value = 20.f;
	APMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(APValue);
	Modifiers.Add(APMod);

	// +15 Armor
	FGameplayModifierInfo ArmorMod;
	ArmorMod.Attribute = UARPGAttributeSet::GetArmorAttribute();
	ArmorMod.ModifierOp = EGameplayModOp::Additive;
	FScalableFloat ArmorValue;
	ArmorValue.Value = 15.f;
	ArmorMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(ArmorValue);
	Modifiers.Add(ArmorMod);

	// Grant State.Buffed.WarCry while active
	FInheritedTagContainer TagContainer;
	TagContainer.AddTag(ARPGGameplayTags::State_Buffed_WarCry);

	UTargetTagsGameplayEffectComponent* TargetTagsComp =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTagsComp);
	TargetTagsComp->SetAndApplyTargetTagChanges(TagContainer);
}

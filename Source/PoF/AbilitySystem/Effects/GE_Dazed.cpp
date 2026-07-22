#include "AbilitySystem/Effects/GE_Dazed.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Dazed::UGE_Dazed()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// 1.6 s — single-sourced from the catalog row status-effects::status-dazed.
	FScalableFloat DurationValue;
	DurationValue.Value = 1.6f;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationValue);

	// Grant State.Dazed while active (mirrors UGE_Stun's TargetTags pattern).
	FInheritedTagContainer TagContainer;
	TagContainer.AddTag(ARPGGameplayTags::State_Dazed);

	UTargetTagsGameplayEffectComponent* TargetTagsComp =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTagsComp);
	TargetTagsComp->SetAndApplyTargetTagChanges(TagContainer);
}

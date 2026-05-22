#include "AbilitySystem/Effects/GE_Stun.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Stun::UGE_Stun()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FScalableFloat DurationValue;
	DurationValue.Value = 2.0f;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationValue);

	// Grant State.Stunned while active
	FInheritedTagContainer TagContainer;
	TagContainer.AddTag(ARPGGameplayTags::State_Stunned);

	UTargetTagsGameplayEffectComponent* TargetTagsComp =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTagsComp);
	TargetTagsComp->SetAndApplyTargetTagChanges(TagContainer);
}

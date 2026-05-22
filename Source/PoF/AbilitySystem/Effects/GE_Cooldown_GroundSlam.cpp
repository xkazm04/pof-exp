#include "AbilitySystem/Effects/GE_Cooldown_GroundSlam.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Cooldown_GroundSlam::UGE_Cooldown_GroundSlam()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FScalableFloat DurationValue;
	DurationValue.Value = 6.0f;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationValue);

	FInheritedTagContainer TagContainer;
	TagContainer.AddTag(ARPGGameplayTags::Cooldown_GroundSlam);

	UTargetTagsGameplayEffectComponent* TargetTagsComp =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTagsComp);
	TargetTagsComp->SetAndApplyTargetTagChanges(TagContainer);
}

#include "AbilitySystem/Effects/GE_Cooldown_ArcaneBeam.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Cooldown_ArcaneBeam::UGE_Cooldown_ArcaneBeam()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FScalableFloat DurationValue;
	DurationValue.Value = 8.0f;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationValue);

	FInheritedTagContainer TagContainer;
	TagContainer.AddTag(ARPGGameplayTags::Cooldown_ArcaneBeam);

	UTargetTagsGameplayEffectComponent* TargetTagsComp =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTagsComp);
	TargetTagsComp->SetAndApplyTargetTagChanges(TagContainer);
}

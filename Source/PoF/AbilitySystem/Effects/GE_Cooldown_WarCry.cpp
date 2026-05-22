#include "AbilitySystem/Effects/GE_Cooldown_WarCry.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Cooldown_WarCry::UGE_Cooldown_WarCry()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FScalableFloat DurationValue;
	DurationValue.Value = 25.0f;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationValue);

	FInheritedTagContainer TagContainer;
	TagContainer.AddTag(ARPGGameplayTags::Cooldown_WarCry);

	UTargetTagsGameplayEffectComponent* TargetTagsComp =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTagsComp);
	TargetTagsComp->SetAndApplyTargetTagChanges(TagContainer);
}

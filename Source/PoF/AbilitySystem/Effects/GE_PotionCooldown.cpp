#include "AbilitySystem/Effects/GE_PotionCooldown.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_PotionCooldown::UGE_PotionCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// Default 1.5s cooldown — the inventory component overrides the duration at apply time
	FScalableFloat DurationValue;
	DurationValue.Value = 1.5f;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationValue);

	// Grant the Cooldown.Potion tag while active
	FInheritedTagContainer TagContainer;
	TagContainer.AddTag(ARPGGameplayTags::Cooldown_Potion);

	UTargetTagsGameplayEffectComponent* TargetTagsComp =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTagsComp);
	TargetTagsComp->SetAndApplyTargetTagChanges(TagContainer);
}

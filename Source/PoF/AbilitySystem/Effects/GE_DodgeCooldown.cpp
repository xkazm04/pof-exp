#include "AbilitySystem/Effects/GE_DodgeCooldown.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_DodgeCooldown::UGE_DodgeCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// 1 second cooldown (tunable via subclass or data)
	FScalableFloat DurationValue;
	DurationValue.Value = 1.0f;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationValue);

	// Grant the Cooldown.Dodge tag while active — GAS queries GetCooldownTags()
	// which reads granted tags from the cooldown GE to determine cooldown status.
	FInheritedTagContainer TagContainer;
	TagContainer.AddTag(ARPGGameplayTags::Cooldown_Dodge);

	UTargetTagsGameplayEffectComponent* TargetTagsComp =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTagsComp);
	TargetTagsComp->SetAndApplyTargetTagChanges(TagContainer);
}

#include "AbilitySystem/GA_UsePotion.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_PotionCooldown.h"
#include "AbilitySystem/Effects/GE_Heal.h"
#include "AbilitySystemComponent.h"

UGA_UsePotion::UGA_UsePotion()
{
	bAutoEndAbility = true;

	// Block while attacking or dashing — don't interrupt active abilities
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Dashing);

	// Cooldown
	CooldownGameplayEffectClass = UGE_PotionCooldown::StaticClass();
	AbilityCooldownTag = ARPGGameplayTags::Cooldown_Potion;

	// No mana cost
	AbilityManaCost = 0.f;

	// Default heal effect
	HealEffect = UGE_Heal::StaticClass();
}

void UGA_UsePotion::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// Super::ActivateAbility calls CommitAbility for us. If it fails, it ends the ability.
	// We override to apply the heal after commit succeeds.

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Apply the heal effect to self
	if (HealEffect)
	{
		ApplyEffectToSelf(HealEffect);
	}

	// End immediately — fire-and-forget
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

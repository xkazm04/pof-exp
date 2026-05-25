#include "AbilitySystem/Abilities/Generated/GA_Gen_Fireball.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/Generated/GE_Gen_Fireball_FireImpact.h"
#include "AbilitySystem/Effects/Generated/GE_Gen_Fireball_Burning.h"
#include "AbilitySystemComponent.h"

UGA_Gen_Fireball::UGA_Gen_Fireball()
{
	bAutoEndAbility = true;
	AbilityManaCost = 20.f;

	// Activation tag rules (from the spec): blocked while incapacitated.
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Stunned);

	// TODO: cooldown GE — out of scope for B3b.
}

void UGA_Gen_Fireball::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Both generated effects are DAMAGING (reduce Health) → apply to the target ASC.
	// TODO: replace this placeholder (the owner's own ASC) with the real target's ASC
	// once target acquisition (projectile / trace / lock-on) is wired — that step is
	// bespoke per ability and cannot be generated from the spec alone.
	if (UAbilitySystemComponent* TargetASC = GetARPGAbilitySystemComponent())
	{
		ApplyEffectToTarget(TargetASC, UGE_Gen_Fireball_FireImpact::StaticClass());
		ApplyEffectToTarget(TargetASC, UGE_Gen_Fireball_Burning::StaticClass());
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

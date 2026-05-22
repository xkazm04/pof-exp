#include "AbilitySystem/GA_HitReact.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "Character/ARPGCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_HitReact::UGA_HitReact()
{
	bAutoEndAbility = false;

	// No cost, no cooldown — hit react is a passive response

	// Block hit react during attacks, dodge, and dashing — don't interrupt active abilities
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Invulnerable);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Dashing);

	// Event-triggered: activated by Event.HitReact sent from PostGameplayEffectExecute
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = ARPGGameplayTags::Event_HitReact;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGA_HitReact::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// No CommitAbility needed — hit react has no cost or cooldown

	AARPGCharacterBase* Character = GetARPGCharacter();

	// Set hit reacting flag so CanMove() blocks movement during the reaction
	if (Character)
	{
		Character->SetHitReacting(true);
	}

	// Use the character's hit react montage if available, fall back to our default
	UAnimMontage* Montage = nullptr;
	if (Character)
	{
		Montage = Character->GetHitReactMontage();
	}
	if (!Montage)
	{
		Montage = HitReactMontage;
	}

	if (!Montage)
	{
		// No montage available — just end
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// Play the hit react montage
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, TEXT("PlayHitReact"), Montage, MontagePlayRate, NAME_None, true);

	MontageTask->OnCompleted.AddDynamic(this, &UGA_HitReact::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_HitReact::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_HitReact::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_HitReact::OnMontageInterrupted);
	MontageTask->ReadyForActivation();
}

void UGA_HitReact::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (AARPGCharacterBase* Character = GetARPGCharacter())
	{
		Character->SetHitReacting(false);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_HitReact::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_HitReact::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

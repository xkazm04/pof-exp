#include "AbilitySystem/GA_WarCry.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_Cooldown_WarCry.h"
#include "AbilitySystem/Effects/GE_Buff_WarCry.h"
#include "Character/ARPGCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UGA_WarCry::UGA_WarCry()
{
	bAutoEndAbility = false;

	SetAssetTags(FGameplayTagContainer(ARPGGameplayTags::Ability_WarCry));
	ActivationOwnedTags.AddTag(ARPGGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Attacking);

	AbilityManaCost = 15.f;
	CooldownGameplayEffectClass = UGE_Cooldown_WarCry::StaticClass();
	AbilityCooldownTag = ARPGGameplayTags::Cooldown_WarCry;
}

void UGA_WarCry::ActivateAbility(
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

	AARPGCharacterBase* Character = GetARPGCharacter();
	if (Character)
	{
		Character->SetAttacking(true);
	}

	// Apply the buff immediately
	if (BuffEffect)
	{
		ApplyEffectToSelf(BuffEffect);
		UE_LOG(LogTemp, Log, TEXT("[GA_WarCry] Applied buff: +20 AttackPower, +15 Armor for 15s"));
	}

	// Spawn VFX placeholder
	if (WarCryVFX && Character)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			Character->GetWorld(), WarCryVFX,
			Character->GetActorLocation(), Character->GetActorRotation());
	}

	// Play war cry montage
	if (WarCryMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("PlayWarCry"), WarCryMontage, MontagePlayRate, NAME_None, true);

		MontageTask->OnCompleted.AddDynamic(this, &UGA_WarCry::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_WarCry::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_WarCry::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_WarCry::OnMontageInterrupted);
		MontageTask->ReadyForActivation();
	}
	else
	{
		// No montage — end immediately (buff is already applied)
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_WarCry::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (AARPGCharacterBase* Character = GetARPGCharacter())
	{
		Character->SetAttacking(false);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_WarCry::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_WarCry::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

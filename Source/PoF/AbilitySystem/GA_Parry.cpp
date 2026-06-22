#include "AbilitySystem/GA_Parry.h"

#include "AbilitySystem/ARPGGameplayTags.h"
#include "Character/ARPGCharacterBase.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

UGA_Parry::UGA_Parry()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	SetAssetTags(FGameplayTagContainer(ARPGGameplayTags::Ability_Parry));
}

void UGA_Parry::ActivateAbility(
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

	if (AARPGCharacterBase* Character = Cast<AARPGCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		Character->SetParrying(true);
	}

	// Play the block pose. Self-heal to the authored AM_Parry if no montage is assigned.
	UAnimMontage* Montage = ParryMontage;
	if (!Montage)
	{
		Montage = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Weapons/AM_Parry.AM_Parry"));
	}
	bool bCanPlay = false;
	if (Montage)
	{
		if (ACharacter* C = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (USkeletalMeshComponent* M = C->GetMesh())
			{
				bCanPlay = M->GetSkeletalMeshAsset() != nullptr && M->GetAnimInstance() != nullptr;
			}
		}
	}
	if (bCanPlay)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("Parry"), Montage, 1.0f, NAME_None, /*bStopWhenAbilityEnds=*/true);
		MontageTask->ReadyForActivation();
	}

	// End the parry window after ParryWindow seconds.
	UAbilityTask_WaitDelay* Delay = UAbilityTask_WaitDelay::WaitDelay(this, ParryWindow);
	Delay->OnFinish.AddDynamic(this, &UGA_Parry::OnWindowElapsed);
	Delay->ReadyForActivation();
}

void UGA_Parry::OnWindowElapsed()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Parry::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (AARPGCharacterBase* Character = Cast<AARPGCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		Character->SetParrying(false);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

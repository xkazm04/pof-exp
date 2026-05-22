#include "AbilitySystem/GA_EnemyRangedAttack.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "Character/ARPGCharacterBase.h"
#include "Combat/ARPGProjectile.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/ARPGAIController.h"

UGA_EnemyRangedAttack::UGA_EnemyRangedAttack()
{
	bAutoEndAbility = false;

	SetAssetTags(FGameplayTagContainer(ARPGGameplayTags::Ability_Enemy_Ranged));
	ActivationOwnedTags.AddTag(ARPGGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Attacking);
}

void UGA_EnemyRangedAttack::ActivateAbility(
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

	if (AARPGCharacterBase* Character = GetARPGCharacter())
	{
		Character->SetAttacking(true);
	}

	// Play cast montage
	if (CastMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("PlayEnemyRanged"), CastMontage, MontagePlayRate, NAME_None, true);

		MontageTask->OnCompleted.AddDynamic(this, &UGA_EnemyRangedAttack::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_EnemyRangedAttack::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_EnemyRangedAttack::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_EnemyRangedAttack::OnMontageInterrupted);
		MontageTask->ReadyForActivation();

		// Listen for release event (reuses Event.MeleeHit as the "fire now" trigger)
		UAbilityTask_WaitGameplayEvent* WaitRelease = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, ARPGGameplayTags::Event_MeleeHit, nullptr, true, true);

		WaitRelease->EventReceived.AddDynamic(this, &UGA_EnemyRangedAttack::OnReleaseEvent);
		WaitRelease->ReadyForActivation();
	}
	else
	{
		// No montage — fire immediately
		SpawnProjectile();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_EnemyRangedAttack::EndAbility(
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

void UGA_EnemyRangedAttack::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_EnemyRangedAttack::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_EnemyRangedAttack::OnReleaseEvent(FGameplayEventData Payload)
{
	SpawnProjectile();
}

void UGA_EnemyRangedAttack::SpawnProjectile()
{
	if (!ProjectileClass) return;

	AARPGCharacterBase* Character = GetARPGCharacter();
	if (!Character) return;

	UAbilitySystemComponent* SourceASC = GetARPGAbilitySystemComponent();
	if (!SourceASC) return;

	// Determine spawn location in world space
	const FTransform CharTransform = Character->GetActorTransform();
	const FVector SpawnLocation = CharTransform.TransformPosition(SpawnOffset);

	// Aim at the target from the blackboard
	FVector AimDirection = Character->GetActorForwardVector();

	AAIController* AIC = Cast<AAIController>(Character->GetController());
	if (AIC)
	{
		UBlackboardComponent* BBComp = AIC->GetBlackboardComponent();
		if (BBComp)
		{
			AActor* Target = Cast<AActor>(BBComp->GetValueAsObject(AARPGAIController::KEY_TargetActor));
			if (Target)
			{
				AimDirection = (Target->GetActorLocation() - SpawnLocation).GetSafeNormal();
			}
		}
	}

	const FRotator SpawnRotation = AimDirection.Rotation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Character;
	SpawnParams.Instigator = Character;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AARPGProjectile* Projectile = Character->GetWorld()->SpawnActor<AARPGProjectile>(
		ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (Projectile)
	{
		Projectile->InitDamage(SourceASC, DamageEffect, BaseDamage, GetAbilityLevel());

		UE_LOG(LogTemp, Log, TEXT("[GA_EnemyRanged] Spawned projectile at %s aimed %s"),
			*SpawnLocation.ToString(), *AimDirection.ToString());
	}
}

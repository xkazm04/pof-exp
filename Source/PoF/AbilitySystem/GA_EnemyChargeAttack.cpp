#include "AbilitySystem/GA_EnemyChargeAttack.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "Character/ARPGCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Engine/OverlapResult.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/ARPGAIController.h"

UGA_EnemyChargeAttack::UGA_EnemyChargeAttack()
{
	bAutoEndAbility = false;

	SetAssetTags(FGameplayTagContainer(ARPGGameplayTags::Ability_Enemy_Charge));
	ActivationOwnedTags.AddTag(ARPGGameplayTags::State_Charging);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Charging);
}

void UGA_EnemyChargeAttack::ActivateAbility(
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
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Character->SetAttacking(true);

	// Set motion warping target from the blackboard target actor
	AAIController* AIC = Cast<AAIController>(Character->GetController());
	if (AIC)
	{
		UBlackboardComponent* BBComp = AIC->GetBlackboardComponent();
		if (BBComp)
		{
			AActor* Target = Cast<AActor>(BBComp->GetValueAsObject(AARPGAIController::KEY_TargetActor));
			if (Target)
			{
				Character->SetAttackWarpTargetFromActor(Target, WarpTargetName);
			}
		}
	}

	// Play the charge montage
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, TEXT("PlayEnemyCharge"), ChargeMontage, MontagePlayRate, NAME_None, true);

	MontageTask->OnCompleted.AddDynamic(this, &UGA_EnemyChargeAttack::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_EnemyChargeAttack::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_EnemyChargeAttack::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_EnemyChargeAttack::OnMontageInterrupted);
	MontageTask->ReadyForActivation();

	// Listen for impact event (reuses Event.MeleeHit as the "impact" trigger from anim notify)
	UAbilityTask_WaitGameplayEvent* WaitImpact = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, ARPGGameplayTags::Event_MeleeHit, nullptr, true, true);

	WaitImpact->EventReceived.AddDynamic(this, &UGA_EnemyChargeAttack::OnImpactEvent);
	WaitImpact->ReadyForActivation();
}

void UGA_EnemyChargeAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (AARPGCharacterBase* Character = GetARPGCharacter())
	{
		Character->SetAttacking(false);
		Character->ClearWarpTarget(WarpTargetName);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_EnemyChargeAttack::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_EnemyChargeAttack::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_EnemyChargeAttack::OnImpactEvent(FGameplayEventData Payload)
{
	PerformAoEDamage();
}

void UGA_EnemyChargeAttack::PerformAoEDamage()
{
	AARPGCharacterBase* Character = GetARPGCharacter();
	if (!Character || !DamageEffect) return;

	UAbilitySystemComponent* SourceASC = GetARPGAbilitySystemComponent();
	if (!SourceASC) return;

	const FVector Origin = Character->GetActorLocation();

	// AoE sphere overlap
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	Character->GetWorld()->OverlapMultiByChannel(
		Overlaps,
		Origin,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(AoERadius),
		QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor) continue;

		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(HitActor);
		UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC) continue;

		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffect, GetAbilityLevel(), Context);
		if (!SpecHandle.IsValid()) continue;

		SpecHandle.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_Damage_Base, BaseDamage);
		SpecHandle.Data->AddDynamicAssetTag(ARPGGameplayTags::Damage_Physical);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

		UE_LOG(LogTemp, Log, TEXT("[GA_EnemyCharge] AoE hit %s for %.1f base damage"), *HitActor->GetName(), BaseDamage);
	}
}

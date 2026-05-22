#include "AbilitySystem/GA_EnemyMeleeAttack.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "Character/ARPGCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Engine/OverlapResult.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

UGA_EnemyMeleeAttack::UGA_EnemyMeleeAttack()
{
	bAutoEndAbility = false;

	SetAssetTags(FGameplayTagContainer(ARPGGameplayTags::Ability_Enemy_Melee));
	ActivationOwnedTags.AddTag(ARPGGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Attacking);
}

void UGA_EnemyMeleeAttack::ActivateAbility(
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

	// Play the swing montage
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, TEXT("PlayEnemyMelee"), SwingMontage, MontagePlayRate, NAME_None, true);

	MontageTask->OnCompleted.AddDynamic(this, &UGA_EnemyMeleeAttack::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_EnemyMeleeAttack::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_EnemyMeleeAttack::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_EnemyMeleeAttack::OnMontageInterrupted);
	MontageTask->ReadyForActivation();

	// Listen for MeleeHit event from AnimNotifyState_HitDetection
	UAbilityTask_WaitGameplayEvent* WaitHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, ARPGGameplayTags::Event_MeleeHit, nullptr, false, true);

	WaitHit->EventReceived.AddDynamic(this, &UGA_EnemyMeleeAttack::OnMeleeHitEvent);
	WaitHit->ReadyForActivation();
}

void UGA_EnemyMeleeAttack::EndAbility(
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

void UGA_EnemyMeleeAttack::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_EnemyMeleeAttack::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_EnemyMeleeAttack::OnMeleeHitEvent(FGameplayEventData Payload)
{
	// If the AnimNotifyState provides a specific target via event, use that.
	// Otherwise fall back to our own front-arc overlap.
	AActor* HitActor = const_cast<AActor*>(Payload.Target.Get());

	if (HitActor)
	{
		// Single target from anim notify hit detection
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(HitActor);
		UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC || !DamageEffect) return;

		UAbilitySystemComponent* SourceASC = GetARPGAbilitySystemComponent();
		if (!SourceASC) return;

		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffect, GetAbilityLevel(), Context);
		if (!SpecHandle.IsValid()) return;

		SpecHandle.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_Damage_Base, BaseDamage);
		SpecHandle.Data->AddDynamicAssetTag(ARPGGameplayTags::Damage_Physical);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
	else
	{
		// No specific target — do the front-arc sweep
		PerformFrontArcDamage();
	}
}

void UGA_EnemyMeleeAttack::PerformFrontArcDamage()
{
	AARPGCharacterBase* Character = GetARPGCharacter();
	if (!Character || !DamageEffect) return;

	UAbilitySystemComponent* SourceASC = GetARPGAbilitySystemComponent();
	if (!SourceASC) return;

	const FVector Origin = Character->GetActorLocation();
	const FVector Forward = Character->GetActorForwardVector();

	// Sphere overlap to find candidates
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	Character->GetWorld()->OverlapMultiByChannel(
		Overlaps,
		Origin,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(HitRadius),
		QueryParams);

	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(HitHalfAngle));

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor) continue;

		// Front-arc check
		const FVector ToTarget = (HitActor->GetActorLocation() - Origin).GetSafeNormal();
		if (FVector::DotProduct(Forward, ToTarget) < CosHalfAngle)
		{
			continue;
		}

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

		UE_LOG(LogTemp, Log, TEXT("[GA_EnemyMelee] Hit %s for %.1f base damage"), *HitActor->GetName(), BaseDamage);
	}
}

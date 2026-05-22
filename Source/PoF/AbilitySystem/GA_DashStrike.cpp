#include "AbilitySystem/GA_DashStrike.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_Cooldown_DashStrike.h"
#include "Character/ARPGCharacterBase.h"
#include "Player/ARPGPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Engine/OverlapResult.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UGA_DashStrike::UGA_DashStrike()
{
	bAutoEndAbility = false;

	SetAssetTags(FGameplayTagContainer(ARPGGameplayTags::Ability_DashStrike));
	ActivationOwnedTags.AddTag(ARPGGameplayTags::State_Dashing);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Dashing);

	AbilityManaCost = 25.f;
	CooldownGameplayEffectClass = UGE_Cooldown_DashStrike::StaticClass();
	AbilityCooldownTag = ARPGGameplayTags::Cooldown_DashStrike;
}

void UGA_DashStrike::ActivateAbility(
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

	// Determine dash direction: toward cursor for player, else forward
	FVector DashDirection = Character->GetActorForwardVector();
	if (AARPGPlayerCharacter* PlayerChar = Cast<AARPGPlayerCharacter>(Character))
	{
		FVector CursorLoc = PlayerChar->GetCursorWorldLocation();
		FVector Dir = CursorLoc - Character->GetActorLocation();
		Dir.Z = 0.f;
		if (!Dir.IsNearlyZero())
		{
			DashDirection = Dir.GetSafeNormal();
			// Snap character rotation to dash direction
			Character->SetActorRotation(DashDirection.Rotation());
		}
	}

	// Record start position for sweep
	DashStartLocation = Character->GetActorLocation();

	// Set motion warping target at DashDistance along dash direction
	FVector TargetLocation = DashStartLocation + DashDirection * DashDistance;
	Character->SetAttackWarpTarget(TargetLocation, WarpTargetName);

	// Spawn trail VFX
	if (DashVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			Character->GetWorld(), DashVFX, DashStartLocation, DashDirection.Rotation());
	}

	// Play dash montage
	if (DashMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("PlayDashStrike"), DashMontage, MontagePlayRate, NAME_None, true);

		MontageTask->OnCompleted.AddDynamic(this, &UGA_DashStrike::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_DashStrike::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_DashStrike::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_DashStrike::OnMontageInterrupted);
		MontageTask->ReadyForActivation();

		// Listen for impact event (sweep damage triggered by anim notify)
		UAbilityTask_WaitGameplayEvent* WaitImpact = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, ARPGGameplayTags::Event_MeleeHit, nullptr, true, true);

		WaitImpact->EventReceived.AddDynamic(this, &UGA_DashStrike::OnImpactEvent);
		WaitImpact->ReadyForActivation();
	}
	else
	{
		PerformSweepDamage();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}

	UE_LOG(LogTemp, Log, TEXT("[GA_DashStrike] Dashing %.0f units in direction %s"), DashDistance, *DashDirection.ToString());
}

void UGA_DashStrike::EndAbility(
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

void UGA_DashStrike::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_DashStrike::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_DashStrike::OnImpactEvent(FGameplayEventData Payload)
{
	PerformSweepDamage();
}

void UGA_DashStrike::PerformSweepDamage()
{
	AARPGCharacterBase* Character = GetARPGCharacter();
	if (!Character || !DamageEffect) return;

	UAbilitySystemComponent* SourceASC = GetARPGAbilitySystemComponent();
	if (!SourceASC) return;

	// Sweep from start to current position
	const FVector EndLocation = Character->GetActorLocation();
	const FVector SweepDir = (EndLocation - DashStartLocation).GetSafeNormal();

	// Use a capsule sweep along the dash path
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	// Use the midpoint as center and half-extent as the radius
	const FVector MidPoint = (DashStartLocation + EndLocation) * 0.5f;
	const float HalfLength = FVector::Dist(DashStartLocation, EndLocation) * 0.5f;

	// Use a box sweep centered on the midpoint for the path
	FCollisionShape SweepShape = FCollisionShape::MakeSphere(SweepRadius + HalfLength);

	Character->GetWorld()->OverlapMultiByChannel(
		Overlaps, MidPoint, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(SweepRadius + HalfLength), QueryParams);

	TSet<AActor*> AlreadyHit;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor || AlreadyHit.Contains(HitActor)) continue;

		// Verify actor is within sweep radius of the line segment
		const FVector ActorLoc = HitActor->GetActorLocation();
		const FVector ClosestPoint = FMath::ClosestPointOnSegment(ActorLoc, DashStartLocation, EndLocation);
		if (FVector::Dist(ActorLoc, ClosestPoint) > SweepRadius + 50.f) continue;

		AlreadyHit.Add(HitActor);

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

		UE_LOG(LogTemp, Log, TEXT("[GA_DashStrike] Sweep hit %s for %.1f base damage"), *HitActor->GetName(), BaseDamage);
	}
}

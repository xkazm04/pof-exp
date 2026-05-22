#include "AbilitySystem/GA_GroundSlam.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_Cooldown_GroundSlam.h"
#include "Character/ARPGCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Engine/OverlapResult.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UGA_GroundSlam::UGA_GroundSlam()
{
	bAutoEndAbility = false;

	SetAssetTags(FGameplayTagContainer(ARPGGameplayTags::Ability_GroundSlam));
	ActivationOwnedTags.AddTag(ARPGGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Attacking);

	AbilityManaCost = 30.f;
	CooldownGameplayEffectClass = UGE_Cooldown_GroundSlam::StaticClass();
	AbilityCooldownTag = ARPGGameplayTags::Cooldown_GroundSlam;
}

void UGA_GroundSlam::ActivateAbility(
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

	if (SlamMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("PlayGroundSlam"), SlamMontage, MontagePlayRate, NAME_None, true);

		MontageTask->OnCompleted.AddDynamic(this, &UGA_GroundSlam::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_GroundSlam::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_GroundSlam::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_GroundSlam::OnMontageInterrupted);
		MontageTask->ReadyForActivation();

		// Listen for impact event
		UAbilityTask_WaitGameplayEvent* WaitImpact = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, ARPGGameplayTags::Event_MeleeHit, nullptr, true, true);

		WaitImpact->EventReceived.AddDynamic(this, &UGA_GroundSlam::OnImpactEvent);
		WaitImpact->ReadyForActivation();
	}
	else
	{
		// No montage — slam immediately
		PerformAoEDamage();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_GroundSlam::EndAbility(
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

void UGA_GroundSlam::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_GroundSlam::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_GroundSlam::OnImpactEvent(FGameplayEventData Payload)
{
	PerformAoEDamage();
}

void UGA_GroundSlam::PerformAoEDamage()
{
	AARPGCharacterBase* Character = GetARPGCharacter();
	if (!Character || !DamageEffect) return;

	UAbilitySystemComponent* SourceASC = GetARPGAbilitySystemComponent();
	if (!SourceASC) return;

	const FVector Origin = Character->GetActorLocation();

	// Spawn impact VFX
	if (ImpactVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			Character->GetWorld(), ImpactVFX, Origin, FRotator::ZeroRotator);
	}

	// AoE sphere overlap
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	Character->GetWorld()->OverlapMultiByChannel(
		Overlaps, Origin, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(AoERadius), QueryParams);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor) continue;

		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(HitActor);
		UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC) continue;

		// Apply damage
		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffect, GetAbilityLevel(), Context);
		if (!SpecHandle.IsValid()) continue;

		SpecHandle.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_Damage_Base, BaseDamage);
		SpecHandle.Data->AddDynamicAssetTag(ARPGGameplayTags::Damage_Physical);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

		// Apply stun
		if (StunEffect)
		{
			FGameplayEffectSpecHandle StunSpecHandle = SourceASC->MakeOutgoingSpec(StunEffect, GetAbilityLevel(), Context);
			if (StunSpecHandle.IsValid())
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*StunSpecHandle.Data.Get(), TargetASC);
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[GA_GroundSlam] AoE hit %s for %.1f base damage + stun"), *HitActor->GetName(), BaseDamage);
	}
}

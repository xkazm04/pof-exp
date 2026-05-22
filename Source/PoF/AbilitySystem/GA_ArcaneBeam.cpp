#include "AbilitySystem/GA_ArcaneBeam.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/Effects/GE_Cooldown_ArcaneBeam.h"
#include "Character/ARPGCharacterBase.h"
#include "Player/ARPGPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"

UGA_ArcaneBeam::UGA_ArcaneBeam()
{
	bAutoEndAbility = false;

	SetAssetTags(FGameplayTagContainer(ARPGGameplayTags::Ability_ArcaneBeam));
	ActivationOwnedTags.AddTag(ARPGGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Attacking);

	AbilityManaCost = 15.f;
	CooldownGameplayEffectClass = UGE_Cooldown_ArcaneBeam::StaticClass();
	AbilityCooldownTag = ARPGGameplayTags::Cooldown_ArcaneBeam;
}

void UGA_ArcaneBeam::ActivateAbility(
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

	// Enable cursor aim so beam tracks mouse direction
	if (AARPGPlayerCharacter* PlayerChar = Cast<AARPGPlayerCharacter>(Character))
	{
		PlayerChar->SetCursorAimEnabled(true);
	}

	ChannelElapsed = 0.f;

	// Play channel montage if available
	if (ChannelMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("PlayArcaneBeam"), ChannelMontage, MontagePlayRate, NAME_None, false);

		MontageTask->OnCompleted.AddDynamic(this, &UGA_ArcaneBeam::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_ArcaneBeam::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_ArcaneBeam::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_ArcaneBeam::OnMontageInterrupted);
		MontageTask->ReadyForActivation();
	}

	// Start damage tick timer
	UWorld* World = Character->GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(
			TickTimerHandle, this, &UGA_ArcaneBeam::PerformBeamTick,
			TickInterval, true, 0.f);

		// Safety max duration timer
		World->GetTimerManager().SetTimer(
			DurationTimerHandle,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			}),
			MaxChannelDuration, false);
	}

	// Initial VFX burst at hand
	if (BeamVFX)
	{
		const FVector SpawnLoc = Character->GetActorLocation() + Character->GetActorForwardVector() * 80.f + FVector(0.f, 0.f, 60.f);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, BeamVFX, SpawnLoc, Character->GetActorRotation());
	}

	UE_LOG(LogTemp, Log, TEXT("[GA_ArcaneBeam] Channel started (%.1fs max, %.1f dmg/tick, %.1f mana/tick)"),
		MaxChannelDuration, DamagePerTick, ManaDrainPerTick);
}

void UGA_ArcaneBeam::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	AARPGCharacterBase* Character = GetARPGCharacter();
	if (Character)
	{
		Character->SetAttacking(false);

		if (AARPGPlayerCharacter* PlayerChar = Cast<AARPGPlayerCharacter>(Character))
		{
			PlayerChar->SetCursorAimEnabled(false);
		}

		UWorld* World = Character->GetWorld();
		if (World)
		{
			World->GetTimerManager().ClearTimer(TickTimerHandle);
			World->GetTimerManager().ClearTimer(DurationTimerHandle);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_ArcaneBeam::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_ArcaneBeam::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_ArcaneBeam::PerformBeamTick()
{
	AARPGCharacterBase* Character = GetARPGCharacter();
	if (!Character || !DamageEffect) return;

	UAbilitySystemComponent* SourceASC = GetARPGAbilitySystemComponent();
	if (!SourceASC) return;

	// Drain mana per tick — end ability if depleted
	if (ManaDrainPerTick > 0.f)
	{
		const UARPGAttributeSet* Attrs = SourceASC->GetSet<UARPGAttributeSet>();
		if (Attrs && Attrs->GetMana() < ManaDrainPerTick)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			return;
		}
		SourceASC->ApplyModToAttribute(UARPGAttributeSet::GetManaAttribute(), EGameplayModOp::Additive, -ManaDrainPerTick);
	}

	ChannelElapsed += TickInterval;

	// Aim direction: toward cursor for player, forward for AI
	FVector AimDirection = Character->GetActorForwardVector();
	const FVector Origin = Character->GetActorLocation() + FVector(0.f, 0.f, 60.f);

	if (AARPGPlayerCharacter* PlayerChar = Cast<AARPGPlayerCharacter>(Character))
	{
		FVector CursorLoc = PlayerChar->GetCursorWorldLocation();
		FVector Dir = CursorLoc - Origin;
		Dir.Z = 0.f;
		if (!Dir.IsNearlyZero())
		{
			AimDirection = Dir.GetSafeNormal();
		}
	}

	const FVector TraceEnd = Origin + AimDirection * BeamRange;

	// Sphere sweep along beam line
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);
	QueryParams.bTraceComplex = false;

	Character->GetWorld()->SweepMultiByChannel(
		HitResults, Origin, TraceEnd, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeSphere(BeamRadius), QueryParams);

	TSet<AActor*> AlreadyHit;
	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || AlreadyHit.Contains(HitActor)) continue;
		AlreadyHit.Add(HitActor);

		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(HitActor);
		UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC) continue;

		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(this);
		Context.AddHitResult(Hit);

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffect, GetAbilityLevel(), Context);
		if (!SpecHandle.IsValid()) continue;

		SpecHandle.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_Damage_Base, DamagePerTick);
		SpecHandle.Data->AddDynamicAssetTag(ARPGGameplayTags::Damage_Lightning);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

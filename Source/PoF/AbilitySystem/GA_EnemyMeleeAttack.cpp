#include "AbilitySystem/GA_EnemyMeleeAttack.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "Character/ARPGCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Engine/OverlapResult.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "AbilitySystem/Effects/GE_Damage.h"

UGA_EnemyMeleeAttack::UGA_EnemyMeleeAttack()
{
	bAutoEndAbility = false;

	SetAssetTags(FGameplayTagContainer(ARPGGameplayTags::Ability_Enemy_Melee));
	ActivationOwnedTags.AddTag(ARPGGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Attacking);

	// Default the damage effect so the raw C++ ability can be granted directly
	// (no config-BP). A Blueprint subclass may still override this.
	DamageEffect = UGE_Damage::StaticClass();
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

	bDamageApplied = false;

	// Set up the MeleeHit listener FIRST, before the montage. It must outlive a
	// montage that fails to play — otherwise EndAbility tears it down before a
	// hit can land.
	UAbilityTask_WaitGameplayEvent* WaitHit = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, ARPGGameplayTags::Event_MeleeHit, nullptr, false, true);
	WaitHit->EventReceived.AddDynamic(this, &UGA_EnemyMeleeAttack::OnMeleeHitEvent);
	WaitHit->ReadyForActivation();

	// Self-heal an empty swing montage (same placeholder issue as the player melee): fall back
	// to the real sword slash so the Sith actually SWINGS instead of doing nothing.
	if (!SwingMontage || SwingMontage->GetPlayLength() <= 0.f)
	{
		// Prefer the custom code-authored slash; fall back to the original.
		UAnimMontage* Slash = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Weapons/AM_SwordSlashC.AM_SwordSlashC"));
		if (!Slash)
		{
			Slash = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Weapons/AM_SwordSlash.AM_SwordSlash"));
		}
		if (Slash)
		{
			SwingMontage = Slash;
		}
	}

	// Determine whether this avatar can actually play the swing montage. On a
	// gray-box character (no SkeletalMesh / AnimInstance) or with an empty
	// montage, PlayMontageAndWait fails internally; we skip it and run a pure
	// timer-driven attack window instead.
	bool bCanPlayMontage = false;
	if (SwingMontage)
	{
		if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
		{
			if (USkeletalMeshComponent* MeshComp = Character->GetMesh())
			{
				bCanPlayMontage =
					MeshComp->GetSkeletalMeshAsset() != nullptr && MeshComp->GetAnimInstance() != nullptr;
			}
		}
	}

	UE_LOG(LogTemp, Verbose, TEXT("[GA_EnemyMelee] swing: canPlayMontage=%d montage=%s len=%.2f"),
		bCanPlayMontage, SwingMontage ? *SwingMontage->GetName() : TEXT("null"),
		SwingMontage ? SwingMontage->GetPlayLength() : 0.f);

	if (bCanPlayMontage)
	{
		// Pre-arm the fallback flag BEFORE ReadyForActivation() so a synchronous
		// OnInterrupted/OnCancelled (empty/zero-length montage with a live
		// AnimInstance) sees bUsingFallbackWindow=true and does NOT call
		// EndAbility — which would destroy the Event.MeleeHit listener. If the
		// montage actually starts, we reset the flag below.
		bUsingFallbackWindow = true;

		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("PlayEnemyMelee"), SwingMontage, MontagePlayRate, NAME_None, true);

		MontageTask->OnCompleted.AddDynamic(this, &UGA_EnemyMeleeAttack::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_EnemyMeleeAttack::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_EnemyMeleeAttack::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_EnemyMeleeAttack::OnMontageInterrupted);
		MontageTask->ReadyForActivation();

		// Confirm the montage actually started; if not (empty montage), fall through.
		UAbilitySystemComponent* ASC = GetARPGAbilitySystemComponent();
		if (ASC && ASC->GetCurrentMontage() == SwingMontage)
		{
			bUsingFallbackWindow = false;
			return;
		}
		// Montage did not start — bUsingFallbackWindow stays true; fall through.
	}

	// Fallback path: no playable montage. Keep the ability (and its hit listener)
	// alive for a fixed window, then perform the front-arc damage and end.
	bUsingFallbackWindow = true;
	UE_LOG(LogTemp, Log,
		TEXT("[GA_EnemyMelee] No playable swing montage; using %.2fs timer-driven attack window."),
		FallbackAttackWindow);

	UAbilityTask_WaitDelay* FallbackTask = UAbilityTask_WaitDelay::WaitDelay(this, FallbackAttackWindow);
	FallbackTask->OnFinish.AddDynamic(this, &UGA_EnemyMeleeAttack::OnFallbackWindowElapsed);
	FallbackTask->ReadyForActivation();
}

void UGA_EnemyMeleeAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Reset transient fallback state so a future activation starts clean.
	bUsingFallbackWindow = false;
	bDamageApplied = false;

	if (AARPGCharacterBase* Character = GetARPGCharacter())
	{
		Character->SetAttacking(false);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_EnemyMeleeAttack::OnMontageCompleted()
{
	// Ignore a stale completed/blend-out callback from a montage that failed to
	// play — the fallback timer keeps the ability (and its hit listener) alive.
	if (bUsingFallbackWindow)
	{
		return;
	}
	// The self-healed swing montage (AM_SwordSlash) carries no MeleeHit AnimNotify, so the
	// notify-driven hit path never fires. Land the front-arc damage on completion if nothing
	// has hit yet — this makes the enemy swing connect with ANY montage, authored or borrowed.
	if (!bDamageApplied)
	{
		PerformFrontArcDamage();
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_EnemyMeleeAttack::OnMontageInterrupted()
{
	// Ignore an interrupt/cancel that is really "the montage never played".
	if (bUsingFallbackWindow)
	{
		return;
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_EnemyMeleeAttack::OnFallbackWindowElapsed()
{
	// The fallback attack window expired — apply the front-arc damage the montage
	// notify would have triggered, then end the ability normally.
	PerformFrontArcDamage();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
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
	if (bDamageApplied)
	{
		return;
	}
	bDamageApplied = true;

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

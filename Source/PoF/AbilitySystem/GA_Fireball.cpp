#include "AbilitySystem/GA_Fireball.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_Cooldown_Fireball.h"
#include "Character/ARPGCharacterBase.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Combat/ARPGProjectile.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Engine/OverlapResult.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

UGA_Fireball::UGA_Fireball()
{
	bAutoEndAbility = false;

	SetAssetTags(FGameplayTagContainer(ARPGGameplayTags::Ability_Fireball));
	ActivationOwnedTags.AddTag(ARPGGameplayTags::State_Attacking);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Attacking);

	AbilityManaCost = 20.f;
	CooldownGameplayEffectClass = UGE_Cooldown_Fireball::StaticClass();
	AbilityCooldownTag = ARPGGameplayTags::Cooldown_Fireball;
}

void UGA_Fireball::ActivateAbility(
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

		// Enable cursor aim so character faces cast direction
		if (AARPGPlayerCharacter* PlayerChar = Cast<AARPGPlayerCharacter>(Character))
		{
			PlayerChar->SetCursorAimEnabled(true);
		}
	}

	// Play cast montage
	if (CastMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, TEXT("PlayFireball"), CastMontage, MontagePlayRate, NAME_None, true);

		MontageTask->OnCompleted.AddDynamic(this, &UGA_Fireball::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &UGA_Fireball::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_Fireball::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_Fireball::OnMontageInterrupted);
		MontageTask->ReadyForActivation();

		// Wait for release event from anim notify
		UAbilityTask_WaitGameplayEvent* WaitRelease = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, ARPGGameplayTags::Event_MeleeHit, nullptr, true, true);

		WaitRelease->EventReceived.AddDynamic(this, &UGA_Fireball::OnReleaseEvent);
		WaitRelease->ReadyForActivation();
	}
	else
	{
		// No montage — fire immediately
		SpawnProjectile();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

void UGA_Fireball::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (AARPGCharacterBase* Character = GetARPGCharacter())
	{
		Character->SetAttacking(false);

		if (AARPGPlayerCharacter* PlayerChar = Cast<AARPGPlayerCharacter>(Character))
		{
			PlayerChar->SetCursorAimEnabled(false);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Fireball::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Fireball::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Fireball::OnReleaseEvent(FGameplayEventData Payload)
{
	SpawnProjectile();
}

void UGA_Fireball::SpawnProjectile()
{
	AARPGCharacterBase* Character = GetARPGCharacter();
	if (!Character || !ProjectileClass) return;

	UAbilitySystemComponent* SourceASC = GetARPGAbilitySystemComponent();
	if (!SourceASC) return;

	// Spawn at offset relative to character
	const FTransform CharTransform = Character->GetActorTransform();
	const FVector SpawnLocation = CharTransform.TransformPosition(SpawnOffset);

	// Aim toward cursor for player, or forward for AI
	FVector AimDirection = Character->GetActorForwardVector();
	if (AARPGPlayerCharacter* PlayerChar = Cast<AARPGPlayerCharacter>(Character))
	{
		FVector CursorLoc = PlayerChar->GetCursorWorldLocation();
		FVector Dir = CursorLoc - SpawnLocation;
		Dir.Z = 0.f;
		if (!Dir.IsNearlyZero())
		{
			AimDirection = Dir.GetSafeNormal();
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

		// Tag the projectile's damage with Damage.Fire for elemental resistance and VFX
		Projectile->AddDamageTag(ARPGGameplayTags::Damage_Fire);

		UE_LOG(LogTemp, Log, TEXT("[GA_Fireball] Spawned fire projectile aimed at %s"), *AimDirection.ToString());
	}

	// Spawn cast VFX placeholder
	if (CastVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			Character->GetWorld(), CastVFX, SpawnLocation, SpawnRotation);
	}
}

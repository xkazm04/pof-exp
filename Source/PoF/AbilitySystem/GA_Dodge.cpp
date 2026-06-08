#include "AbilitySystem/GA_Dodge.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_DodgeCooldown.h"
#include "Character/ARPGCharacterBase.h"
#include "Player/ARPGPlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UGA_Dodge::UGA_Dodge()
{
	bAutoEndAbility = false;

	// Tag this ability for identification & activation by tag
	SetAssetTags(FGameplayTagContainer(ARPGGameplayTags::Ability_Dodge));

	// Grant State.Invulnerable while active — GAS adds on activate, removes on end
	ActivationOwnedTags.AddTag(ARPGGameplayTags::State_Invulnerable);

	// Block re-activation while already dodging
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Invulnerable);

	// Can't dodge while attacking or stunned (State_Dead/Stunned already blocked by base class)
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Attacking);

	// Cooldown — CommitAbility will apply this GE automatically.
	// GAS checks cooldown by querying GetCooldownTags(), which reads
	// the tags from the cooldown GE's InheritableOwnedTagsContainer.
	CooldownGameplayEffectClass = UGE_DodgeCooldown::StaticClass();
	AbilityCooldownTag = ARPGGameplayTags::Cooldown_Dodge;
}

bool UGA_Dodge::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const AARPGCharacterBase* Character = ActorInfo
		? Cast<AARPGCharacterBase>(ActorInfo->AvatarActor.Get())
		: nullptr;

	if (!Character)
	{
		return false;
	}

	// Must have enough stamina
	if (Character->GetStamina() < StaminaCost)
	{
		return false;
	}

	// Must be grounded (respect the character's setting)
	if (Character->RequiresGroundedForDodge())
	{
		const UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
		if (MoveComp && !MoveComp->IsMovingOnGround())
		{
			return false;
		}
	}

	return true;
}

void UGA_Dodge::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// CommitAbility handles cooldown GE (CooldownGameplayEffectClass if assigned)
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

	// Consume stamina
	Character->ConsumeStamina(StaminaCost);

	// --- Determine dodge direction from movement input ---
	const FVector LastInput = Character->GetLastMovementInputVector();
	FVector DodgeDir;
	if (!LastInput.IsNearlyZero())
	{
		DodgeDir = FVector(LastInput.X, LastInput.Y, 0.f).GetSafeNormal();
	}
	else if (const AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(Character))
	{
		// Stationary player — dodge toward the cursor aim point.
		FVector ToCursor = Player->GetCursorWorldLocation() - Player->GetActorLocation();
		ToCursor.Z = 0.f;
		DodgeDir = ToCursor.GetSafeNormal();
		if (DodgeDir.IsNearlyZero())
		{
			DodgeDir = Player->GetActorForwardVector();
			DodgeDir.Z = 0.f;
			DodgeDir = DodgeDir.GetSafeNormal();
		}
	}
	else
	{
		// Non-player (AI) — dodge backward relative to facing.
		DodgeDir = -Character->GetActorForwardVector();
		DodgeDir.Z = 0.f;
		DodgeDir = DodgeDir.GetSafeNormal();
	}

	// Classify into 4-way direction
	// Use the character's public method to get the right montage
	const FVector Forward = Character->GetActorForwardVector();
	const FVector Right = Character->GetActorRightVector();
	const float ForwardDot = FVector::DotProduct(DodgeDir, Forward);
	const float RightDot = FVector::DotProduct(DodgeDir, Right);

	if (FMath::Abs(ForwardDot) >= FMath::Abs(RightDot))
	{
		ResolvedDirection = ForwardDot >= 0.f ? EARPGDodgeDirection::Forward : EARPGDodgeDirection::Backward;
	}
	else
	{
		ResolvedDirection = RightDot >= 0.f ? EARPGDodgeDirection::Right : EARPGDodgeDirection::Left;
	}

	// Snap rotation to dodge direction
	Character->SetActorRotation(DodgeDir.Rotation());

	// Enter the dodge state. AM_Roll's root motion (now enabled) drives the travel, so the
	// capsule follows the animation exactly (no foot-slide) — we just flag the state here.
	Character->BeginDodge();

	// Select the montage for this direction (uses character's assigned montages)
	UAnimMontage* Montage = Character->GetDodgeMontageForDirection(ResolvedDirection);

	if (!Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_Dodge] No dodge montage assigned for direction %d"), static_cast<int32>(ResolvedDirection));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Broadcast dodge started delegate for VFX/SFX
	Character->OnDodgeStarted.Broadcast(ResolvedDirection, DodgeDir);

	// Play the montage — root motion from the montage drives the character's movement
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		TEXT("PlayDodge"),
		Montage,
		Character->GetDodgeMontagePlayRate(),
		NAME_None,
		/*bStopWhenAbilityEnds=*/ true);

	MontageTask->OnCompleted.AddDynamic(this, &UGA_Dodge::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_Dodge::OnMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_Dodge::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_Dodge::OnMontageCancelled);
	MontageTask->ReadyForActivation();

	UE_LOG(LogTemp, Log, TEXT("[GA_Dodge] Activated: Direction=%d"), static_cast<int32>(ResolvedDirection));
}

void UGA_Dodge::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// Clear the dodge state (restores movement/facing) — also broadcasts OnDodgeEnded.
	if (AARPGCharacterBase* Character = GetARPGCharacter())
	{
		Character->EndDodge();
	}

	// State.Invulnerable tag is automatically removed by GAS when EndAbility runs
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Dodge::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Dodge::OnMontageBlendOut()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Dodge::OnMontageInterrupted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Dodge::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

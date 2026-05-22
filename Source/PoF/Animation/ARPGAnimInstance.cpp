#include "Animation/ARPGAnimInstance.h"
#include "Character/ARPGCharacterBase.h"
#include "GameFramework/CharacterMovementComponent.h"

UARPGAnimInstance::UARPGAnimInstance()
{
	// Root motion from montages only — locomotion animations (blend spaces)
	// use velocity-driven movement, while attack/dodge montages use root motion.
	RootMotionMode = ERootMotionMode::RootMotionFromMontagesOnly;
}

void UARPGAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	OwnerCharacter = Cast<AARPGCharacterBase>(TryGetPawnOwner());
}

void UARPGAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!OwnerCharacter)
	{
		OwnerCharacter = Cast<AARPGCharacterBase>(TryGetPawnOwner());
		if (!OwnerCharacter)
		{
			return;
		}
	}

	const UCharacterMovementComponent* MoveComp = OwnerCharacter->GetCharacterMovement();

	// =================================================================
	// Core locomotion
	// =================================================================
	Speed = OwnerCharacter->GetVelocity().Size2D();
	Direction = OwnerCharacter->GetMovementDirection();
	bIsInAir = MoveComp ? MoveComp->IsFalling() : false;

	SpeedRatio = OwnerCharacter->GetSpeedRatio();
	LocomotionState = OwnerCharacter->GetLocomotionState();
	bIsSprinting = OwnerCharacter->IsSprinting();
	bShouldMove = Speed > 10.f && !bIsInAir;

	// =================================================================
	// Combat state
	// =================================================================
	bIsDodging = OwnerCharacter->IsDodging();
	bIsUsingRootMotion = OwnerCharacter->IsUsingRootMotion();
	bIsAttacking = OwnerCharacter->IsAttacking();
	bIsHitReacting = OwnerCharacter->IsHitReacting();
	bIsComboWindowOpen = OwnerCharacter->IsComboWindowOpen();
	DodgeDirection = OwnerCharacter->GetDodgeDirection();

	// Full-body montage: any state where root motion should drive the character
	// and locomotion blending should be suppressed in the AnimBP.
	bIsFullBodyMontage = bIsAttacking || bIsDodging || bIsHitReacting;

	// Death state — query the character's virtual IsDead() (overridden in Player/Enemy subclasses).
	bIsDead = OwnerCharacter->IsDead();

	// =================================================================
	// Transition interrupt rules
	// =================================================================

	// Dodge cancel window: can chain dodge -> attack during the late phase
	bCanInterruptDodge = bIsDodging && OwnerCharacter->IsInDodgeCancelWindow();

	// Attack recovery: attack montage is playing but the combo window has closed.
	// This means the active hit frames are over and the character is in the
	// "wind-down" portion — a natural point to allow dodge cancelling.
	bIsAttackRecovery = bIsAttacking && !bIsComboWindowOpen && bIsAnyMontageActive;

	// Dodge cancels attack recovery: allows the "dodge out of late attack" pattern.
	// True when the player is attacking in recovery AND a dodge input arrives.
	bDodgeCancelsAttack = bIsAttackRecovery && bIsDodging;

	// Hit-react interrupt: one-frame pulse when hit-react begins.
	// This lets the AnimBP trigger an immediate transition from any state -> HitReact.
	bHitReactInterrupt = bIsHitReacting && !bWasHitReacting;
	bWasHitReacting = bIsHitReacting;

	// =================================================================
	// Run state (lean, sprint blend, transitions)
	// =================================================================
	UpdateRunState(DeltaSeconds);

	// =================================================================
	// Upper-body blending
	// =================================================================
	// bIsUpperBodyAction is currently driven by gameplay code. When set,
	// we smoothly blend the upper-body layer weight toward 1.0 so the
	// locomotion legs continue playing underneath.
	const float TargetWeight = bIsUpperBodyAction ? 1.f : 0.f;
	UpperBodyBlendWeight = FMath::FInterpTo(UpperBodyBlendWeight, TargetWeight, DeltaSeconds, 10.f);

	// =================================================================
	// State machine
	// =================================================================
	PreviousAnimState = AnimState;
	AnimState = ComputeAnimState();

	// Track time in current state
	if (AnimState == PreviousAnimState)
	{
		StateTime += DeltaSeconds;
	}
	else
	{
		StateTime = 0.f;
	}

	// =================================================================
	// Montage tracking
	// =================================================================
	UpdateMontageInfo();
}

EARPGAnimState UARPGAnimInstance::ComputeAnimState() const
{
	// Priority order: Death > HitReact > Dodging > Attacking > Locomotion
	// This ensures the most critical states always win.

	if (bIsDead)
	{
		return EARPGAnimState::Death;
	}

	if (bIsHitReacting)
	{
		return EARPGAnimState::HitReact;
	}

	if (bIsDodging)
	{
		return EARPGAnimState::Dodging;
	}

	if (bIsAttacking)
	{
		return EARPGAnimState::Attacking;
	}

	return EARPGAnimState::Locomotion;
}

void UARPGAnimInstance::UpdateRunState(float DeltaSeconds)
{
	// --- Running flag ---
	bIsRunning = (LocomotionState == 2);

	// --- Lean angle from direction change rate ---
	// Compute angular velocity (degrees per second) accounting for -180/+180 wrapping
	float DirectionDelta = Direction - PreviousDirection;

	// Handle wrap-around (e.g., -170 to 170 should be -20, not +340)
	if (DirectionDelta > 180.f) DirectionDelta -= 360.f;
	else if (DirectionDelta < -180.f) DirectionDelta += 360.f;

	TurnRate = (DeltaSeconds > SMALL_NUMBER) ? FMath::Abs(DirectionDelta / DeltaSeconds) : 0.f;

	// Target lean: proportional to direction change rate, scaled by speed
	// Only lean when actually moving at run speed
	const float SpeedFactor = FMath::Clamp(SpeedRatio, 0.f, 1.f);
	const float RawLean = (DeltaSeconds > SMALL_NUMBER)
		? FMath::Clamp(DirectionDelta / DeltaSeconds * 0.1f, -MaxLeanAngle, MaxLeanAngle)
		: 0.f;
	const float TargetLean = RawLean * SpeedFactor;

	// Smooth the lean to avoid jitter
	LeanAngle = FMath::FInterpTo(LeanAngle, TargetLean, DeltaSeconds, LeanInterpSpeed);

	PreviousDirection = Direction;

	// --- Normalized run speed ---
	// Map SpeedRatio from [RunSpeedThreshold..1.0] to [0..1]
	const float RunThreshold = OwnerCharacter ? OwnerCharacter->GetRunSpeedThreshold() : 0.7f;
	if (SpeedRatio > RunThreshold)
	{
		RunSpeedNormalized = FMath::Clamp((SpeedRatio - RunThreshold) / FMath::Max(1.f - RunThreshold, 0.01f), 0.f, 1.f);
	}
	else
	{
		RunSpeedNormalized = 0.f;
	}

	// --- Sprint blend alpha ---
	const float SprintTarget = bIsSprinting ? 1.f : 0.f;
	SprintBlendAlpha = FMath::FInterpTo(SprintBlendAlpha, SprintTarget, DeltaSeconds, SprintBlendInterpSpeed);

	// --- Stamina ratio ---
	StaminaRatio = OwnerCharacter ? OwnerCharacter->GetStaminaRatio() : 1.f;

	// --- Run -> Walk transition ---
	// True when we were running but speed dropped below the run threshold
	bRunToWalkTransition = bIsRunning && (SpeedRatio < RunThreshold);

	// --- Locomotion -> Jump transition ---
	// True on the frame the character becomes airborne while in locomotion
	bLocomotionToJumpTransition = bIsInAir && !bWasInAir && (AnimState == EARPGAnimState::Locomotion);

	// --- Just landed ---
	bJustLanded = !bIsInAir && bWasInAir;

	bWasInAir = bIsInAir;
}

void UARPGAnimInstance::UpdateMontageInfo()
{
	const UAnimMontage* CurrentMontage = GetCurrentActiveMontage();
	if (CurrentMontage)
	{
		bIsAnyMontageActive = true;
		const float MontageLength = CurrentMontage->GetPlayLength();
		const float MontagePosition = Montage_GetPosition(CurrentMontage);

		if (MontageLength > 0.f)
		{
			ActiveMontageProgress = FMath::Clamp(MontagePosition / MontageLength, 0.f, 1.f);
			ActiveMontageTimeRemaining = FMath::Max(MontageLength - MontagePosition, 0.f);
		}
		else
		{
			ActiveMontageProgress = 0.f;
			ActiveMontageTimeRemaining = 0.f;
		}

		// Track the current section name for AnimBP section-aware transitions
		ActiveMontageSection = Montage_GetCurrentSection(CurrentMontage);
	}
	else
	{
		bIsAnyMontageActive = false;
		ActiveMontageProgress = 0.f;
		ActiveMontageTimeRemaining = 0.f;
		ActiveMontageSection = NAME_None;
	}

	// Root motion velocity tracking — useful for blending locomotion
	// in/out at root motion boundaries
	if (bIsUsingRootMotion && OwnerCharacter)
	{
		const FVector Velocity = OwnerCharacter->GetVelocity();
		RootMotionSpeed = Velocity.Size();
		RootMotionDirection = Velocity.GetSafeNormal();
	}
	else
	{
		RootMotionSpeed = 0.f;
		RootMotionDirection = FVector::ZeroVector;
	}
}

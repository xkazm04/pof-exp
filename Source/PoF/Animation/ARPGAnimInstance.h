#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Character/ARPGCharacterBase.h"
#include "ARPGAnimInstance.generated.h"

class AARPGCharacterBase;

/**
 * Active animation state — mirrors the 5-state AnimBP state machine.
 * Computed in C++ each frame so the AnimBP can use a single enum for
 * transition rules instead of checking multiple booleans.
 */
UENUM(BlueprintType)
enum class EARPGAnimState : uint8
{
	Locomotion,
	Attacking,
	Dodging,
	HitReact,
	Death
};

/**
 * Custom anim instance that caches locomotion variables every frame
 * for use by the Animation Blueprint state machine and blend spaces.
 *
 * State machine layout (5 states):
 *   Locomotion (default) <-> Attacking <-> Dodging
 *                         -> HitReact  -> Death
 *
 * Transition rules (use these BlueprintReadOnly variables):
 *   Locomotion -> Attacking:   bIsAttacking == true
 *   Locomotion -> Dodging:     bIsDodging == true
 *   Locomotion -> HitReact:    bIsHitReacting == true
 *   Locomotion -> Death:       bIsDead == true
 *   Attacking  -> Locomotion:  bIsAttacking == false && !bIsFullBodyMontage
 *   Attacking  -> Dodging:     bIsDodging == true  (dodge cancels attack recovery)
 *   Attacking  -> HitReact:    bIsHitReacting == true (hit interrupts attack)
 *   Dodging    -> Locomotion:  bIsDodging == false
 *   Dodging    -> Attacking:   bIsAttacking == true && bCanInterruptDodge (cancel window)
 *   HitReact   -> Locomotion:  bIsHitReacting == false
 *   HitReact   -> Death:       bIsDead == true
 *   Death      -> (terminal):  no exit transitions
 *
 * Upper-body blending:
 *   Use UpperBodyBlendWeight in a Layered Blend per Bone node to overlay
 *   upper-body actions (e.g., casting, item use) over locomotion.
 *   Cache the locomotion pose with "Save Cached Pose" -> "CachedLocomotion",
 *   then feed it into the Layered Blend as the base layer.
 */
UCLASS()
class POF_API UARPGAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UARPGAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// =================================================================
	// State Machine
	// =================================================================

	/** Current high-level animation state — drives the AnimBP state machine. */
	UPROPERTY(BlueprintReadOnly, Category = "StateMachine")
	EARPGAnimState AnimState = EARPGAnimState::Locomotion;

	/** Previous frame's state — used for detecting transitions in AnimBP. */
	UPROPERTY(BlueprintReadOnly, Category = "StateMachine")
	EARPGAnimState PreviousAnimState = EARPGAnimState::Locomotion;

	/** Seconds spent in the current AnimState. Useful for transition delays. */
	UPROPERTY(BlueprintReadOnly, Category = "StateMachine")
	float StateTime = 0.f;

	// =================================================================
	// Locomotion
	// =================================================================

	/** Absolute ground speed (cm/s). */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Speed = 0.f;

	/** Movement direction relative to character facing (-180 to 180). */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Direction = 0.f;

	/** Whether the character is falling / jumping. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsInAir = false;

	/** Whether the character is currently sprinting. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bIsSprinting = false;

	/** Speed as a 0-1 ratio of MaxWalkSpeed. Useful for blend spaces. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float SpeedRatio = 0.f;

	/** 0 = Idle, 1 = Walk, 2 = Run/Sprint. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	int32 LocomotionState = 0;

	/** Whether the character has meaningful velocity (speed > idle threshold). */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	bool bShouldMove = false;

	// =================================================================
	// Run State (sub-state of Locomotion)
	// =================================================================

	/** True when LocomotionState == 2 (Run/Sprint). Convenience flag for AnimBP. */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Run")
	bool bIsRunning = false;

	/**
	 * Lean angle in degrees (-LeanMax to +LeanMax).
	 * Derived from the rate of direction change: positive = leaning right, negative = leaning left.
	 * Use as a blend space axis for run lean animations.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Run")
	float LeanAngle = 0.f;

	/**
	 * Rate of direction change in degrees/sec (absolute value).
	 * Useful for driving procedural lean intensity or foot plant adjustments.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Run")
	float TurnRate = 0.f;

	/**
	 * Normalized run speed: 0 at walk-to-run threshold, 1 at sprint max.
	 * Maps the speed range [RunThreshold..SprintMax] to [0..1].
	 * Use as the horizontal axis in a run blend space.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Run")
	float RunSpeedNormalized = 0.f;

	/**
	 * Smooth 0-1 blend between run (0) and sprint (1).
	 * Interpolates toward 1 when sprinting, 0 when not.
	 * Use to blend between run and sprint pose layers or blend spaces.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Run")
	float SprintBlendAlpha = 0.f;

	/**
	 * Current stamina as a 0-1 ratio. When low, sprint animations
	 * can blend toward fatigued/labored variants.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Run")
	float StaminaRatio = 1.f;

	/**
	 * True when the character's speed has dropped below the run threshold
	 * while currently in the run state. Use for Run -> Walk transition rules.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Run")
	bool bRunToWalkTransition = false;

	/**
	 * True when the character becomes airborne during locomotion.
	 * Use for Run -> Jump (or Walk -> Jump) transition rules.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Run")
	bool bLocomotionToJumpTransition = false;

	/**
	 * True when the character just landed (was in air last frame, grounded now).
	 * Use for Jump -> Locomotion transition rules.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion|Run")
	bool bJustLanded = false;

	// =================================================================
	// Combat State
	// =================================================================

	/** Whether the character is currently attacking (playing attack montage). */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsAttacking = false;

	/** Whether the character is dodging. */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDodging = false;

	/** Whether the character is playing a hit-reaction montage. */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsHitReacting = false;

	/** Whether the character is dead (gameplay sets this on the character). */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	/** Whether root motion is active on the character. */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsUsingRootMotion = false;

	/**
	 * True when a full-body root motion montage is playing (attack, dodge, hit-react).
	 * Use this in the AnimBP to decide whether to suppress locomotion blending.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsFullBodyMontage = false;

	/** Current dodge direction (valid when bIsDodging is true). */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	EARPGDodgeDirection DodgeDirection = EARPGDodgeDirection::Forward;

	/** Whether the combo input window is currently open. */
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsComboWindowOpen = false;

	// =================================================================
	// Transition Interrupt Rules
	// =================================================================

	/**
	 * True when the current dodge is in its cancel window.
	 * Allows Dodging -> Attacking transition for responsiveness.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Transitions")
	bool bCanInterruptDodge = false;

	/**
	 * True when the attack is in recovery (combo window closed, montage still playing).
	 * Allows Attacking -> Dodging and Attacking -> Locomotion transitions for responsiveness.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Transitions")
	bool bIsAttackRecovery = false;

	/**
	 * True when a hit-react should interrupt the current state.
	 * This is a one-frame pulse that fires on hit-react entry.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Transitions")
	bool bHitReactInterrupt = false;

	/**
	 * True when a dodge should interrupt attack recovery.
	 * Allows the classic "cancel late attack recovery into dodge" action RPG pattern.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Transitions")
	bool bDodgeCancelsAttack = false;

	// =================================================================
	// Upper-Body Blending (Cached Pose)
	// =================================================================

	/**
	 * Blend weight for upper-body layer overlay [0, 1].
	 * In the AnimBP:
	 *   1. Create a "Save Cached Pose" node named "CachedLocomotion" from the locomotion output.
	 *   2. Use a "Layered Blend per Bone" node:
	 *      - Base Pose: "Use Cached Pose: CachedLocomotion"
	 *      - Blend Pose 0: Your upper-body montage / state output
	 *      - Branch filter bone: "spine_02" (or Mixamo "Spine2")
	 *      - Alpha: UpperBodyBlendWeight
	 * This lets locomotion legs play under upper-body actions (e.g., casting while running).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Blending")
	float UpperBodyBlendWeight = 0.f;

	/**
	 * Whether the character is performing an upper-body-only action
	 * (legs should continue playing locomotion underneath).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Blending")
	bool bIsUpperBodyAction = false;

	// =================================================================
	// Montage Helpers
	// =================================================================

	/** Remaining time on the current active montage (0 if none). */
	UPROPERTY(BlueprintReadOnly, Category = "Montage")
	float ActiveMontageTimeRemaining = 0.f;

	/** Fractional progress [0,1] of the current active montage. */
	UPROPERTY(BlueprintReadOnly, Category = "Montage")
	float ActiveMontageProgress = 0.f;

	/** Whether any montage is currently playing on the default slot. */
	UPROPERTY(BlueprintReadOnly, Category = "Montage")
	bool bIsAnyMontageActive = false;

	/** Name of the currently active montage section (e.g., "Attack1", "Dodge"). Empty if none. */
	UPROPERTY(BlueprintReadOnly, Category = "Montage")
	FName ActiveMontageSection = NAME_None;

	// =================================================================
	// Root Motion
	// =================================================================

	/**
	 * Root motion velocity magnitude (cm/s) from the current montage.
	 * Useful for blending locomotion in/out during root motion transitions.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RootMotion")
	float RootMotionSpeed = 0.f;

	/**
	 * Root motion direction in world space (normalized).
	 * Can drive directional blend transitions when exiting root motion montages.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RootMotion")
	FVector RootMotionDirection = FVector::ZeroVector;

private:
	UPROPERTY()
	TObjectPtr<AARPGCharacterBase> OwnerCharacter;

	/** Tracks whether hit-react was active last frame for pulse detection. */
	bool bWasHitReacting = false;

	/** Previous frame's direction — used to compute direction change rate for lean. */
	float PreviousDirection = 0.f;

	/** Previous frame's airborne state — used for landing detection. */
	bool bWasInAir = false;

	/** Max lean angle in degrees. Lean is clamped to [-MaxLean, +MaxLean]. */
	static constexpr float MaxLeanAngle = 30.f;

	/** Interp speed for lean smoothing (higher = snappier lean response). */
	static constexpr float LeanInterpSpeed = 8.f;

	/** Interp speed for sprint blend alpha (smooth run<->sprint transition). */
	static constexpr float SprintBlendInterpSpeed = 5.f;

	/** Compute the high-level animation state from individual flags. */
	EARPGAnimState ComputeAnimState() const;

	/** Update montage tracking variables. */
	void UpdateMontageInfo();

	/** Update run-specific state (lean, sprint blend, transition flags). */
	void UpdateRunState(float DeltaSeconds);
};

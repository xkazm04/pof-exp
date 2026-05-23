#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "GA_EnemyMeleeAttack.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * Enemy melee attack ability.
 *
 * Flow:
 *   1. Plays a swing montage
 *   2. At the hit window (anim notify), performs a front-arc sphere overlap
 *   3. Applies GE_Damage to all targets in the arc
 *   4. Ends when the montage completes or is cancelled
 *
 * Unlike the player's GA_MeleeAttack, this has no combo system —
 * the BT handles attack sequencing via cooldown.
 */
UCLASS()
class POF_API UGA_EnemyMeleeAttack : public UARPGGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_EnemyMeleeAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** The swing montage to play. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Melee")
	TObjectPtr<UAnimMontage> SwingMontage;

	/** Play rate for the swing montage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Melee", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MontagePlayRate = 1.0f;

	/** Damage GE class (should be GE_Damage or subclass). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Melee|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** Base damage per swing. Passed as SetByCaller Data.Damage.Base. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Melee|Damage")
	float BaseDamage = 15.f;

	/** Radius of the front-arc overlap check (from character origin). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Melee|HitDetection", meta = (ClampMin = "50"))
	float HitRadius = 200.f;

	/** Half-angle in degrees of the front arc (90 = full 180-degree arc). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Melee|HitDetection", meta = (ClampMin = "10", ClampMax = "180"))
	float HitHalfAngle = 90.f;

	/** Attack-window length (s) used when no swing montage can play (gray-box avatar / empty montage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Melee", meta = (ClampMin = "0.05"))
	float FallbackAttackWindow = 0.3f;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMeleeHitEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnFallbackWindowElapsed();

	void PerformFrontArcDamage();

	/** True while running the timer-driven fallback (no playable swing montage). Pre-armed before ReadyForActivation. */
	bool bUsingFallbackWindow = false;

	/** Guard so the fallback front-arc damage applies at most once per activation. */
	bool bDamageApplied = false;
};

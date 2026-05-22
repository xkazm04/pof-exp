#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "GA_DashStrike.generated.h"

class UAnimMontage;
class UGameplayEffect;
class UNiagaraSystem;

/**
 * Dash Strike — dash forward with Motion Warping, damaging all enemies in path.
 *
 * Flow:
 *   1. CommitAbility (mana cost + cooldown)
 *   2. Set motion warp target at cursor direction * DashDistance
 *   3. Play dash montage (contains MotionWarping notify for "DashTarget")
 *   4. On Event.MeleeHit (from notify at impact frame):
 *      capsule sweep from start to current position, damage all overlapped enemies
 *   5. Clear warp target, end ability when montage finishes
 */
UCLASS()
class POF_API UGA_DashStrike : public UARPGGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_DashStrike();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DashStrike")
	TObjectPtr<UAnimMontage> DashMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DashStrike", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DashStrike|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DashStrike|Damage")
	float BaseDamage = 40.f;

	/** How far the dash travels. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DashStrike|Dash", meta = (ClampMin = "100"))
	float DashDistance = 800.f;

	/** Radius of the capsule sweep for hitting enemies along the path. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DashStrike|Dash", meta = (ClampMin = "50"))
	float SweepRadius = 150.f;

	/** Warp target name (must match Motion Warping notify in the montage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DashStrike|MotionWarping")
	FName WarpTargetName = TEXT("DashTarget");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DashStrike|VFX")
	TObjectPtr<UNiagaraSystem> DashVFX;

private:
	FVector DashStartLocation = FVector::ZeroVector;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnImpactEvent(FGameplayEventData Payload);

	void PerformSweepDamage();
};

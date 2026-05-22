#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "GA_EnemyChargeAttack.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * Enemy charge attack ability (used by Brute archetype).
 *
 * Flow:
 *   1. Sets a Motion Warping target on the owning character aimed at the BB target
 *   2. Plays a charge montage that includes a MotionWarping notify window
 *   3. On the hit event (montage anim notify), performs an AoE sphere overlap
 *   4. Applies GE_Damage to all targets within the AoE radius
 *   5. Clears the warp target and ends when the montage finishes
 *
 * The montage must contain a Motion Warping notify with target name "ChargeTarget".
 */
UCLASS()
class POF_API UGA_EnemyChargeAttack : public UARPGGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_EnemyChargeAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** The charge montage. Must include a Motion Warping notify for "ChargeTarget". */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Charge")
	TObjectPtr<UAnimMontage> ChargeMontage;

	/** Play rate for the charge montage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Charge", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MontagePlayRate = 1.0f;

	/** Damage GE class (should be GE_Damage or subclass). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Charge|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** Base damage for the AoE impact. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Charge|Damage")
	float BaseDamage = 40.f;

	/** Radius of the AoE damage sphere on arrival. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Charge|AoE", meta = (ClampMin = "100"))
	float AoERadius = 300.f;

	/** Warp target name (must match the Motion Warping notify in the montage). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Charge|MotionWarping")
	FName WarpTargetName = TEXT("ChargeTarget");

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnImpactEvent(FGameplayEventData Payload);

	void PerformAoEDamage();
};

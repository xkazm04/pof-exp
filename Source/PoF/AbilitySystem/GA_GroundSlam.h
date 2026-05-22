#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "GA_GroundSlam.generated.h"

class UAnimMontage;
class UGameplayEffect;
class UNiagaraSystem;

/**
 * Ground Slam — AoE slam centered on the player dealing physical damage
 * and applying a short stun to nearby enemies.
 *
 * Flow:
 *   1. CommitAbility (mana cost + cooldown)
 *   2. Play slam montage
 *   3. On Event.MeleeHit (impact frame from anim notify):
 *      sphere overlap for AoE damage + apply stun GE
 *   4. End ability when montage finishes
 */
UCLASS()
class POF_API UGA_GroundSlam : public UARPGGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_GroundSlam();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GroundSlam")
	TObjectPtr<UAnimMontage> SlamMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GroundSlam", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GroundSlam|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GroundSlam|Damage")
	float BaseDamage = 50.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GroundSlam|AoE", meta = (ClampMin = "100"))
	float AoERadius = 400.f;

	/** Stun GE applied to enemies caught in the slam. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GroundSlam|Stun")
	TSubclassOf<UGameplayEffect> StunEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GroundSlam|VFX")
	TObjectPtr<UNiagaraSystem> ImpactVFX;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnImpactEvent(FGameplayEventData Payload);

	void PerformAoEDamage();
};

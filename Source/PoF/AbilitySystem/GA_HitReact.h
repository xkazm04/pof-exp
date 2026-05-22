#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "GA_HitReact.generated.h"

class UAnimMontage;

/**
 * Hit reaction ability — activated by Event.HitReact from PostGameplayEffectExecute.
 *
 * Flow:
 *   1. PostGameplayEffectExecute detects non-lethal damage → sends Event.HitReact
 *   2. GA_HitReact activates via event trigger
 *   3. Plays a hit react montage (directional if configured)
 *   4. Ends when the montage completes
 *
 * This ability is blocked by State.Attacking and State.Invulnerable so attacks
 * and dodges are not interrupted by incoming hits. State.Dead blocks via base class.
 */
UCLASS()
class POF_API UGA_HitReact : public UARPGGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_HitReact();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** Default hit react montage (plays when no directional montage is available). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact")
	TObjectPtr<UAnimMontage> HitReactMontage;

	/** Play rate for the hit react montage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitReact", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MontagePlayRate = 1.0f;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();
};

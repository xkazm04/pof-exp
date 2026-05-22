#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "GA_ArcaneBeam.generated.h"

class UAnimMontage;
class UGameplayEffect;
class UNiagaraSystem;

/**
 * Arcane Beam — channeled ability that deals sustained damage in a line.
 *
 * Flow:
 *   1. CommitAbility (mana cost + cooldown)
 *   2. Enable cursor aim so beam tracks cursor direction
 *   3. Play channel montage (looping section)
 *   4. While channeling: tick damage via line trace every TickInterval
 *   5. Drains mana per tick — ends when mana depleted, cancelled, or MaxChannelDuration reached
 *   6. End ability, disable cursor aim
 */
UCLASS()
class POF_API UGA_ArcaneBeam : public UARPGGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ArcaneBeam();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcaneBeam")
	TObjectPtr<UAnimMontage> ChannelMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcaneBeam", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcaneBeam|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** Damage per tick. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcaneBeam|Damage")
	float DamagePerTick = 12.f;

	/** How far the beam reaches. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcaneBeam|Beam", meta = (ClampMin = "100"))
	float BeamRange = 1200.f;

	/** Radius of the beam's sphere trace. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcaneBeam|Beam", meta = (ClampMin = "10"))
	float BeamRadius = 40.f;

	/** Time between damage ticks. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcaneBeam|Channel", meta = (ClampMin = "0.05"))
	float TickInterval = 0.25f;

	/** Maximum channel duration. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcaneBeam|Channel", meta = (ClampMin = "0.5"))
	float MaxChannelDuration = 4.0f;

	/** Mana drained per tick while channeling. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcaneBeam|Cost", meta = (ClampMin = "0"))
	float ManaDrainPerTick = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ArcaneBeam|VFX")
	TObjectPtr<UNiagaraSystem> BeamVFX;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	void PerformBeamTick();

	FTimerHandle TickTimerHandle;
	FTimerHandle DurationTimerHandle;
	float ChannelElapsed = 0.f;
};

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "GA_WarCry.generated.h"

class UAnimMontage;
class UGameplayEffect;
class UNiagaraSystem;

/**
 * War Cry — self-buff that increases AttackPower (+20) and Armor (+15) for 15 seconds.
 *
 * Flow:
 *   1. CommitAbility (mana cost + cooldown)
 *   2. Play war cry montage (roar animation)
 *   3. Apply GE_Buff_WarCry to self (duration-based, auto-removes)
 *   4. Spawn VFX placeholder
 *   5. End ability when montage finishes
 */
UCLASS()
class POF_API UGA_WarCry : public UARPGGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_WarCry();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WarCry")
	TObjectPtr<UAnimMontage> WarCryMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WarCry", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MontagePlayRate = 1.0f;

	/** The buff GE to apply to self (GE_Buff_WarCry). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WarCry|Buff")
	TSubclassOf<UGameplayEffect> BuffEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WarCry|VFX")
	TObjectPtr<UNiagaraSystem> WarCryVFX;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();
};

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "GA_Parry.generated.h"

class UAnimMontage;

/**
 * Saber parry/block (player ability, hotbar slot 2 / key '2').
 *
 * Raises the blade into a block pose and marks the character IsParrying() for a short window.
 * The SaberClashSubsystem turns a blade crossing during that window (against an attacker) into
 * a successful parry: it cancels the attacker's swing + spawns the clash FX. Pure timing —
 * no damage, no targeting; the window is what matters.
 */
UCLASS()
class POF_API UGA_Parry : public UARPGGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Parry();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** How long IsParrying() stays true after activation (the deflect window). */
	UPROPERTY(EditDefaultsOnly, Category = "Parry", meta = (ClampMin = "0.1"))
	float ParryWindow = 0.45f;

	/** Block pose montage; falls back to /Game/Weapons/AM_Parry if unset. */
	UPROPERTY(EditDefaultsOnly, Category = "Parry")
	TObjectPtr<UAnimMontage> ParryMontage;

	UFUNCTION()
	void OnWindowElapsed();
};

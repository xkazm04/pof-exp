#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "Character/ARPGCharacterBase.h"
#include "GA_Dodge.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;

/**
 * Dodge roll gameplay ability.
 *
 * Flow:
 *   1. ActivateAbility -> CommitAbility (cooldown) -> consume stamina
 *   2. Determine dodge direction from character's last movement input
 *   3. Snap rotation to dodge direction
 *   4. PlayMontageAndWait with directional montage (root motion drives movement)
 *   5. State.Invulnerable granted via ActivationOwnedTags while ability is active
 *   6. OnMontageCompleted/Interrupted/Cancelled -> EndAbility -> invulnerability removed
 *
 * Montage setup:
 *   Assign directional dodge montages on AARPGCharacterBase (or override here).
 *   Montages should have root motion for the roll displacement.
 */
UCLASS()
class POF_API UGA_Dodge : public UARPGGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Dodge();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** Stamina cost consumed on activation (uses character's stamina pool, not GAS Mana). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge")
	float StaminaCost = 25.f;

	/** Play rate for dodge montages. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dodge", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MontagePlayRate = 1.0f;

private:
	/** The direction resolved at activation. */
	EARPGDodgeDirection ResolvedDirection = EARPGDodgeDirection::Forward;

	// --- Montage callbacks ---
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();
};

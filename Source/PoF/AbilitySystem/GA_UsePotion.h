#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "GA_UsePotion.generated.h"

class UGameplayEffect;

/**
 * Potion use ability — instant heal with cooldown.
 *
 * Flow:
 *   1. CommitAbility (cooldown via GE_PotionCooldown)
 *   2. Apply GE_Heal to self
 *   3. Fire GameplayCue.Heal for VFX/SFX
 *   4. End ability immediately (fire-and-forget)
 *
 * This ability has no mana cost — it uses the potion cooldown to gate usage.
 */
UCLASS()
class POF_API UGA_UsePotion : public UARPGGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_UsePotion();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** The heal effect to apply. Should be GE_Heal or a subclass. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Potion")
	TSubclassOf<UGameplayEffect> HealEffect;
};

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "ARPGGameplayAbility.generated.h"

class AARPGCharacterBase;
class UARPGAttributeSet;
class UTexture2D;

UCLASS(Abstract)
class POF_API UARPGGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UARPGGameplayAbility();

	/** Get the owning character (avatar actor cast to our base). */
	UFUNCTION(BlueprintPure, Category = "Ability")
	AARPGCharacterBase* GetARPGCharacter() const;

	/** Get the owning ASC. */
	UFUNCTION(BlueprintPure, Category = "Ability")
	UAbilitySystemComponent* GetARPGAbilitySystemComponent() const;

	/** Get the attribute set from the owning ASC. */
	UFUNCTION(BlueprintPure, Category = "Ability")
	const UARPGAttributeSet* GetARPGAttributeSet() const;

	/**
	 * Apply a gameplay effect class to a target ASC using this ability's context.
	 * @param TargetASC  The target's AbilitySystemComponent.
	 * @param EffectClass  The gameplay effect class to apply.
	 * @param Level  Effect level (defaults to ability level).
	 * @return The active effect handle, invalid if application failed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability")
	FActiveGameplayEffectHandle ApplyEffectToTarget(UAbilitySystemComponent* TargetASC, TSubclassOf<UGameplayEffect> EffectClass, float Level = -1.f);

	/**
	 * Apply a gameplay effect class to self.
	 * @param EffectClass  The gameplay effect class to apply.
	 * @param Level  Effect level (defaults to ability level).
	 * @return The active effect handle.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability")
	FActiveGameplayEffectHandle ApplyEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass, float Level = -1.f);

	// --- Cost overrides ---
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

	// --- Loadout / UI accessors (read from CDO) ---

	/** Mana cost for this ability. */
	UFUNCTION(BlueprintPure, Category = "Ability|Cost")
	float GetAbilityManaCost() const { return AbilityManaCost; }

	/** Cooldown tag that this ability's cooldown GE grants. Used by the HUD ability bar. */
	UFUNCTION(BlueprintPure, Category = "Ability|Cooldown")
	FGameplayTag GetAbilityCooldownTag() const { return AbilityCooldownTag; }

	/** Icon for this ability (displayed in the hotbar). */
	UFUNCTION(BlueprintPure, Category = "Ability|UI")
	UTexture2D* GetAbilityIcon() const { return AbilityIcon; }

protected:
	// --- Activation policy ---

	/** If true, the ability ends itself immediately after ActivateAbility executes (fire-and-forget). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Activation")
	bool bAutoEndAbility = false;

	/** Mana cost for this ability. 0 = no mana cost. Deducted from the Mana attribute on commit. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Cost", meta = (ClampMin = "0"))
	float AbilityManaCost = 0.f;

	/** Cooldown tag that this ability's cooldown GE grants. Set in ability constructor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Cooldown")
	FGameplayTag AbilityCooldownTag;

	/** Icon texture for the HUD hotbar. Assign in Blueprint or constructor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|UI")
	TObjectPtr<UTexture2D> AbilityIcon;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IARPGDefaultsProvider.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class UInputMappingContext;

/** An IMC the actor wants added to the Enhanced Input subsystem on possession. */
USTRUCT(BlueprintType)
struct FARPGDefaultInputContext
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> Context = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	int32 Priority = 0;
};

UINTERFACE(MinimalAPI, BlueprintType)
class UARPGDefaultsProvider : public UInterface { GENERATED_BODY() };

/**
 * Declares the defaults a possessed actor applies on PossessedBy. A new gameplay
 * system is wired by appending an entry to one of these arrays - never by
 * editing PossessedBy. Implementers return references to their own arrays;
 * callers (e.g. AARPGCharacterBase::PossessedBy) iterate and apply.
 *
 * Scope: applies-on-possession only. Runtime activation (input bindings, GAS
 * activation) is out of scope for this contract.
 */
class IARPGDefaultsProvider
{
	GENERATED_BODY()
public:
	/** Abilities to grant to the ASC (server-authoritative). */
	virtual const TArray<TSubclassOf<UGameplayAbility>>& GetDefaultAbilities() const = 0;

	/** Persistent gameplay effects to apply to self on possession (e.g. startup attribute boosts). */
	virtual const TArray<TSubclassOf<UGameplayEffect>>& GetDefaultEffects() const = 0;

	/** Input Mapping Contexts the player setup will add to the Enhanced Input subsystem. */
	virtual const TArray<FARPGDefaultInputContext>& GetDefaultInputContexts() const = 0;
};

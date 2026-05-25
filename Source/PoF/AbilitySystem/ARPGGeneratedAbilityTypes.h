#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "ARPGGeneratedAbilityTypes.generated.h"

class UARPGGameplayAbility;
class UGameplayEffect;

/**
 * One row of DT_GeneratedAbilities — the additive registry of abilities produced by
 * the app's "Generate C++" dispatch (ECW B3c). Discovery surface only; does NOT touch
 * the hand-authored DT_AbilityCatalog / FARPGAbilityCatalogRow / its lookup library.
 */
USTRUCT(BlueprintType)
struct FARPGGeneratedAbilityRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Display name of the source ability (e.g. "Fireball"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generated Ability")
	FName Name;

	/** The ability's gameplay tag (may be unregistered for app-only abilities). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generated Ability")
	FGameplayTag GameplayTag;

	/** The generated UGA_Gen_* ability class. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generated Ability")
	TSoftClassPtr<UARPGGameplayAbility> AbilityClass;

	/** The generated UGE_Gen_* effect classes the ability applies. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Generated Ability")
	TArray<TSoftClassPtr<UGameplayEffect>> EffectClasses;
};

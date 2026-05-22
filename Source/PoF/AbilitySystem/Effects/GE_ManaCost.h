#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_ManaCost.generated.h"

/**
 * Instant cost effect that deducts Mana via SetByCaller "Data.ManaCost".
 * Used as CostGameplayEffectClass on abilities with mana costs.
 */
UCLASS()
class POF_API UGE_ManaCost : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_ManaCost();
};

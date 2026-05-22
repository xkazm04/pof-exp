#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_PotionCooldown.generated.h"

/**
 * Cooldown effect for consumable potion usage.
 * Grants the Cooldown.Potion tag for its duration,
 * preventing spam-use of consumables.
 */
UCLASS()
class POF_API UGE_PotionCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_PotionCooldown();
};

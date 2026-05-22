#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_DodgeCooldown.generated.h"

/**
 * Cooldown effect for the dodge ability.
 * Duration-based: applies a cooldown tag for the configured duration.
 * Assigned as CooldownGameplayEffectClass on GA_Dodge.
 */
UCLASS()
class POF_API UGE_DodgeCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_DodgeCooldown();
};

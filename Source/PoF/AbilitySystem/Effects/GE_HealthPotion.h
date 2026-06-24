#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_HealthPotion.generated.h"

/**
 * Instant heal effect for the Minor Health Potion consumable — writes to the
 * IncomingHeal meta attribute (PostGameplayEffectExecute clamps to MaxHealth,
 * broadcasts heal numbers, and fires GameplayCue.Heal).
 * Default magnitude: +50 HP (a half-heal: 50 -> 100 at the default 100 MaxHealth).
 * Authored as a dedicated GE so the shared +25 GE_Heal stays untouched.
 */
UCLASS()
class POF_API UGE_HealthPotion : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_HealthPotion();
};

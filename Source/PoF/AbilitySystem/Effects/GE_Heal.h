#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Heal.generated.h"

/**
 * Instant heal effect — writes to IncomingHeal meta attribute.
 * PostGameplayEffectExecute applies it to Health (clamped to MaxHealth),
 * broadcasts damage numbers, and fires GameplayCue.Heal.
 * Default magnitude: +25 HP.
 */
UCLASS()
class POF_API UGE_Heal : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Heal();
};

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_DifficultyScaling.generated.h"

/**
 * Infinite-duration GE that scales enemy Health, MaxHealth, and AttackPower.
 *
 * All three modifiers use Multiply operation with a SetByCaller magnitude
 * keyed on Data.DifficultyMultiplier. The spawner sets this value based
 * on the player-to-enemy level delta.
 *
 * Example: if the multiplier is 1.3, the enemy gets +30% to all three stats.
 */
UCLASS()
class POF_API UGE_DifficultyScaling : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_DifficultyScaling();
};

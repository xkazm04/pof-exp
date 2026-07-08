#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGAchievementRules.generated.h"

/**
 * Achievement unlock rules for "First Blood" (catalog pipeline: achievements).
 * Threshold 1 kill; idempotent unlock (fires exactly once); reward is 100 gold
 * (currency-gold) + 1 Minor Health Potion (item-7). Mirrors
 * src/lib/catalog/pipelines/achievements.ts.
 */
UCLASS()
class POF_API UARPGAchievementRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr int32 FirstBloodThreshold = 1;
	static constexpr int32 RewardGold = 100;
	static constexpr int32 RewardItemQuantity = 1; // item-7 Minor Health Potion

	/** True only when the threshold is met AND it has not already been granted
	 *  (idempotent: a second call after unlock returns false). */
	UFUNCTION(BlueprintPure, Category = "Achievements")
	static bool ShouldUnlock(int32 KillCount, int32 Threshold, bool bAlreadyUnlocked);

	/** The reward-item id granted on unlock. */
	UFUNCTION(BlueprintPure, Category = "Achievements")
	static FName RewardItemId() { return FName(TEXT("item-7")); }
};

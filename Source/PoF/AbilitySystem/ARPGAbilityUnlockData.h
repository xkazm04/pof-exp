#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Abilities/GameplayAbility.h"
#include "ARPGAbilityUnlockData.generated.h"

/**
 * Per-rank stat increase applied when upgrading an ability.
 * Each rank beyond 1 adds these bonuses cumulatively.
 */
USTRUCT(BlueprintType)
struct POF_API FAbilityRankBonus
{
	GENERATED_BODY()

	/** Extra base damage added per rank (for damage abilities). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rank")
	float BonusDamage = 0.f;

	/** Cooldown reduction per rank (seconds subtracted from base cooldown). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rank")
	float CooldownReduction = 0.f;

	/** Mana cost reduction per rank. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rank")
	float ManaCostReduction = 0.f;

	/** Generic bonus multiplier per rank (1.0 = no change, 1.1 = +10% per rank). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rank")
	float BonusMultiplier = 1.0f;
};

/**
 * Data Table row defining a learnable/upgradeable ability.
 *
 * Create a Data Table asset (DT_AbilityUnlocks) with this row struct.
 * Each row represents one ability that the player can learn and upgrade.
 * Row name should be a descriptive identifier (e.g., "Fireball", "GroundSlam").
 */
USTRUCT(BlueprintType)
struct POF_API FAbilityUnlockRow : public FTableRowBase
{
	GENERATED_BODY()

	/** The gameplay ability class to grant when learned. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	TSubclassOf<UGameplayAbility> AbilityClass;

	/** Minimum player level required to learn this ability. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements", meta = (ClampMin = "1"))
	int32 RequiredLevel = 1;

	/** Skill points required to learn this ability (rank 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Requirements", meta = (ClampMin = "1"))
	int32 SkillPointCost = 1;

	/** Maximum rank this ability can be upgraded to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxRank = 3;

	/** Skill points required per upgrade (each rank beyond 1 costs this many). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade", meta = (ClampMin = "1"))
	int32 UpgradeSkillPointCost = 1;

	/** Stat increases applied per rank beyond rank 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FAbilityRankBonus PerRankBonus;

	/** Display name for UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FText DisplayName;

	/** Description for UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	FText Description;
};

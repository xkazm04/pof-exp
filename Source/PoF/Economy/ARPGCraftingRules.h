#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGCraftingRules.generated.h"

/**
 * Crafting gate rules for the Alchemist's Bench health-potion recipe (catalog
 * pipeline: crafting-recipes). Bench upgrades the base 50 HP potion to 120 HP
 * (2.4x Tier-1 quality mult), costs 20g, requires Crafting Skill 1 at an alchemist
 * station with all reagents. Mirrors src/lib/catalog/pipelines/crafting-recipes.ts.
 */
UCLASS()
class POF_API UARPGCraftingRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr int32 BaseSeedHeal = 50;   // item-7 base drop
	static constexpr int32 GoldCost = 20;
	static constexpr int32 RequiredSkill = 1;

	/** Bench-crafted heal amount = round(BaseSeedHeal * 2.4) = 120 HP. */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	static int32 CraftedHealAmount() { return FMath::RoundToInt(static_cast<double>(BaseSeedHeal) * 2.4); }

	/** All gating rules for a successful craft (gold, skill, station, reagents). */
	UFUNCTION(BlueprintPure, Category = "Crafting")
	static bool CanCraft(int32 PlayerGold, int32 CraftingSkill, bool bAtAlchemistBench, bool bHasAllReagents);
};

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGAffixRoller.generated.h"

class UARPGItemDefinition;
class UARPGItemInstance;
class UDataTable;
struct FItemAffix;
enum class EARPGItemRarity : uint8;

/**
 * Static utility for rolling random affixes on item instances.
 * Uses weighted random selection with rarity tier-gating.
 * Rarity determines how many affixes are rolled:
 *   Common    = 0
 *   Uncommon  = 1-2
 *   Rare      = 3-4
 *   Epic      = 4-5
 *   Legendary = 5-6
 */
UCLASS()
class POF_API UARPGAffixRoller : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Roll a set of random affixes from the given Data Table based on rarity.
	 * Uses weighted selection with tier-gating (affixes with MinRarity > Rarity are excluded).
	 * Magnitude is randomized between the row's MinValue and MaxValue, then scaled by ItemLevel.
	 *
	 * @param AffixPool   Data Table with FAffixTableRow rows.
	 * @param Rarity      Determines how many affixes to roll and which affixes are eligible.
	 * @param ItemLevel   Scales affix magnitude: magnitude * (1 + 0.1 * ItemLevel).
	 * @param OutAffixes  Filled with the rolled affixes. Cleared on entry.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Affixes")
	static void RollAffixes(const UDataTable* AffixPool, EARPGItemRarity Rarity, int32 ItemLevel, TArray<FItemAffix>& OutAffixes);

	/**
	 * Reroll all affixes on an item instance from its definition's affix pool.
	 * Useful for crafting systems. Preserves the item's rarity and level.
	 *
	 * @param Item  The item to reroll affixes on.
	 * @return true if affixes were rerolled successfully.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Affixes")
	static bool RerollAffixes(UARPGItemInstance* Item);

	/** Get the min/max affix count for a given rarity tier. */
	static void GetAffixCountRange(EARPGItemRarity Rarity, int32& OutMin, int32& OutMax);

	/** Calculate item-level scaling multiplier: 1.0 + 0.1 * ItemLevel. */
	static float GetItemLevelScaling(int32 ItemLevel);
};

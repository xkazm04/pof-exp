#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/ARPGItemDefinition.h"
#include "ARPGLootTable.generated.h"

class UARPGItemInstance;

USTRUCT(BlueprintType)
struct FLootEntry
{
	GENERATED_BODY()

	/** The item definition to drop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TObjectPtr<UARPGItemDefinition> Item;

	/** Relative weight for weighted random selection. Higher = more likely. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0.0"))
	float DropWeight = 1.f;

	/** Minimum quantity dropped (inclusive). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "1"))
	int32 MinQuantity = 1;

	/** Maximum quantity dropped (inclusive). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "1"))
	int32 MaxQuantity = 1;

	/** Minimum rarity tier the dropped item can roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	EARPGItemRarity MinRarity = EARPGItemRarity::Common;

	/** Maximum rarity tier the dropped item can roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	EARPGItemRarity MaxRarity = EARPGItemRarity::Legendary;
};

/**
 * Data asset defining a weighted loot table.
 * Each entry has a drop weight; a NothingWeight adds chance of no drop.
 * Call RollLoot() to perform weighted random selection and get item instances.
 */
UCLASS(BlueprintType)
class POF_API UARPGLootTable : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Loot entries with weighted drop chances. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TArray<FLootEntry> Entries;

	/** Weight for the "nothing drops" outcome. 0 = always drops something. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0.0"))
	float NothingWeight = 0.f;

	/**
	 * Perform a weighted random roll on this loot table.
	 * Returns an array of newly created UARPGItemInstance objects.
	 * May return empty if the "nothing" outcome is selected.
	 *
	 * @param Outer  The UObject that will own the created item instances (e.g. the inventory component or world).
	 * @param RarityBonusMultiplier  Multiplier that biases rarity toward higher tiers (1.0 = normal, 2.0 = much better drops).
	 * @return Array of item instances from the roll. Empty if nothing dropped.
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	TArray<UARPGItemInstance*> RollLoot(UObject* Outer, float RarityBonusMultiplier = 1.f) const;

	/**
	 * Roll a rarity within the given range, weighted so lower rarities are more common.
	 * Base weights: Common=100, Uncommon=50, Rare=20, Epic=5, Legendary=1.
	 * RarityBonusMultiplier raises the weight of higher tiers.
	 */
	static EARPGItemRarity RollWeightedRarity(EARPGItemRarity MinRarity, EARPGItemRarity MaxRarity, float RarityBonusMultiplier = 1.f);

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};

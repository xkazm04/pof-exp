#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ARPGLootDropComponent.generated.h"

class UARPGLootTable;
class AARPGWorldItem;
class AARPGEnemyCharacter;
class UARPGItemInstance;
class UARPGItemDefinition;

/**
 * Component that handles loot dropping on death.
 * Attach to any actor (typically enemies) that should drop loot.
 * Rolls the loot table and spawns AARPGWorldItem actors around the owner.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class POF_API UARPGLootDropComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGLootDropComponent();

	/** The loot table to roll on death. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	TObjectPtr<UARPGLootTable> LootTable;

	/** Number of independent rolls on the loot table. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "1"))
	int32 NumRolls = 1;

	/** Bonus rarity weight multiplier (e.g., 2.0 = twice as likely to get higher rarities). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0.0"))
	float RarityBonusMultiplier = 1.f;

	/** Radius around the owner to scatter dropped items. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0"))
	float DropScatterRadius = 100.f;

	/** Height offset above owner for dropped items. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0"))
	float DropHeightOffset = 50.f;

	/** Whether to automatically drop loot when the owner dies (binds to OnEnemyDeath). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	bool bAutoDropOnDeath = true;

	// =====================================================================
	// Gold Drop
	// =====================================================================

	/** Whether to drop a gold pickup on death (separate from loot-table items). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Gold")
	bool bDropGold = true;

	/** Base gold amount dropped per enemy level (before random variance). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Gold", meta = (ClampMin = "0"))
	int32 BaseGoldPerLevel = 10;

	/** Flat gold added on top of the per-level amount. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Gold", meta = (ClampMin = "0"))
	int32 FlatGoldBonus = 5;

	/** Random variance applied to the gold amount (0.4 = ±40%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Gold", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float GoldVariance = 0.4f;

	/**
	 * Item definition representing a gold pickup. Optional — if left null, a
	 * transient definition is created at runtime so the slice still works.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Gold")
	TObjectPtr<UARPGItemDefinition> GoldItemDefinition;

	// =====================================================================
	// Slice Mode
	// =====================================================================

	/**
	 * Vertical-slice mode. When true, dropped world items self-destruct after
	 * SliceLifeSpan seconds instead of relying on inventory pickup, so no
	 * UARPGInventoryComponent is required on the player.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Slice")
	bool bSliceMode = true;

	/** Lifetime (seconds) for dropped world items in slice mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Slice", meta = (ClampMin = "0.0"))
	float SliceLifeSpan = 60.f;

	/**
	 * Roll the loot table and spawn world items around the owner.
	 * Also drops a gold pickup if bDropGold is set.
	 * @return The spawned world items (including the gold pickup).
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	TArray<AARPGWorldItem*> DropLoot();

	/**
	 * Spawn a gold pickup scaled to the given enemy level.
	 * @return The spawned gold world item, or null on failure.
	 */
	UFUNCTION(BlueprintCallable, Category = "Loot|Gold")
	AARPGWorldItem* DropGold(int32 EnemyLevel);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnOwnerDeath(AARPGEnemyCharacter* Enemy);

	AARPGWorldItem* SpawnWorldItem(UARPGItemInstance* Instance, int32 Index, int32 Total);

	/** Spawn a world item near the owner and launch it with the given velocity. */
	AARPGWorldItem* SpawnWorldItemInternal(UARPGItemInstance* Instance, FVector LaunchVelocity);

	/** Resolve (or lazily create) the item definition used for gold pickups. */
	UARPGItemDefinition* ResolveGoldDefinition();

	/** Cached transient gold definition, created on demand when none is assigned. */
	UPROPERTY(Transient)
	TObjectPtr<UARPGItemDefinition> RuntimeGoldDefinition;
};

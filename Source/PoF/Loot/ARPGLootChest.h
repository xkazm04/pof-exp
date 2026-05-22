#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGLootChest.generated.h"

class UARPGLootTable;
class UStaticMeshComponent;
class USphereComponent;
class AARPGWorldItem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChestOpened, AARPGLootChest*, Chest);

/**
 * Interactive loot chest. Player interacts to open, which plays an
 * animation (timeline-driven lid rotation) and spawns loot from a table.
 */
UCLASS()
class POF_API AARPGLootChest : public AActor
{
	GENERATED_BODY()

public:
	AARPGLootChest();

	/** Attempt to open the chest. Returns false if already opened. */
	UFUNCTION(BlueprintCallable, Category = "Loot|Chest")
	bool TryOpen(AActor* Opener);

	UFUNCTION(BlueprintPure, Category = "Loot|Chest")
	bool IsOpened() const { return bIsOpened; }

	UPROPERTY(BlueprintAssignable, Category = "Events|Loot")
	FOnChestOpened OnChestOpened;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> LidMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionTrigger;

	/** Loot table to roll when opened. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Chest")
	TObjectPtr<UARPGLootTable> LootTable;

	/** Number of rolls on the loot table. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Chest", meta = (ClampMin = "1"))
	int32 NumRolls = 2;

	/** Distance items scatter from the chest. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Chest", meta = (ClampMin = "0"))
	float ItemScatterRadius = 150.f;

	/** Lid open angle in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Chest|Animation")
	float LidOpenAngle = -110.f;

	/** How fast the lid opens (degrees per second). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Chest|Animation", meta = (ClampMin = "1"))
	float LidOpenSpeed = 180.f;

	/** If true, the chest can be opened again after a delay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Chest")
	bool bCanRespawn = false;

	/** Time in seconds before the chest resets (if bCanRespawn). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Chest", meta = (EditCondition = "bCanRespawn", ClampMin = "1"))
	float RespawnDelay = 300.f;

private:
	bool bIsOpened = false;
	bool bIsAnimating = false;
	float CurrentLidAngle = 0.f;
	float TargetLidAngle = 0.f;

	FTimerHandle RespawnTimerHandle;

	void SpawnLoot();
	void ResetChest();
};

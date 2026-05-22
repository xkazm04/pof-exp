#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ARPGZoneDefinition.generated.h"

class AARPGEnemyCharacter;

/** Difficulty tier for a zone — drives enemy scaling, loot quality, etc. */
UENUM(BlueprintType)
enum class EZoneDifficulty : uint8
{
	Safe     UMETA(DisplayName = "Safe (Town/Hub)"),
	Easy     UMETA(DisplayName = "Easy"),
	Medium   UMETA(DisplayName = "Medium"),
	Hard     UMETA(DisplayName = "Hard"),
	Boss     UMETA(DisplayName = "Boss")
};

/** A single encounter group within a zone — defines what enemies spawn together. */
USTRUCT(BlueprintType)
struct FZoneEncounterGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	FName GroupName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter")
	TArray<TSubclassOf<AARPGEnemyCharacter>> EnemyClasses;

	/** Min/max enemies from this group per encounter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter", meta = (ClampMin = "0"))
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter", meta = (ClampMin = "1"))
	int32 MaxCount = 3;

	/** Weight for random selection among groups. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter", meta = (ClampMin = "0.01"))
	float SelectionWeight = 1.0f;
};

/** Connection to another zone — describes how zones link together. */
USTRUCT(BlueprintType)
struct FZoneConnection
{
	GENERATED_BODY()

	/** The zone this connects to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	TSoftObjectPtr<UARPGZoneDefinition> ConnectedZone;

	/** Minimum player level recommended for traversal. 0 = no gate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection", meta = (ClampMin = "0"))
	int32 RecommendedLevel = 0;

	/** If true, requires a key item or quest completion to unlock. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection")
	bool bRequiresUnlock = false;

	/** Quest ID that must be completed to unlock this connection. Empty = no quest gate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Connection", meta = (EditCondition = "bRequiresUnlock"))
	FName RequiredQuestID;
};

/**
 * Data asset that defines a single world zone.
 * Place one per zone to drive spawning, difficulty, music, and connectivity.
 */
UCLASS(BlueprintType)
class POF_API UARPGZoneDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Unique identifier for this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FName ZoneID;

	/** Display name shown in UI. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FText ZoneDisplayName;

	/** Difficulty tier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	EZoneDifficulty Difficulty = EZoneDifficulty::Easy;

	/** Recommended player level range for this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone", meta = (ClampMin = "1"))
	int32 RecommendedLevelMin = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone", meta = (ClampMin = "1"))
	int32 RecommendedLevelMax = 10;

	/** Base enemy level in this zone (used by spawn points/volumes). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Difficulty", meta = (ClampMin = "1"))
	int32 BaseEnemyLevel = 1;

	/** Difficulty multiplier applied to all enemies in this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Difficulty", meta = (ClampMin = "1.0"))
	float ZoneDifficultyMultiplier = 1.0f;

	/** Encounter groups available in this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Encounters")
	TArray<FZoneEncounterGroup> EncounterGroups;

	/** Max concurrent enemies alive in this zone. 0 = unlimited. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Encounters", meta = (ClampMin = "0"))
	int32 MaxConcurrentEnemies = 20;

	/** Connections to other zones (defines the world graph). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Connections")
	TArray<FZoneConnection> Connections;

	/** Level streaming name to load for this zone. Empty = zone is in persistent level. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone|Streaming")
	FName StreamingLevelName;

	/** Gameplay tag representing this zone (e.g. Zone.Forest, Zone.Catacombs). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	FGameplayTag ZoneTag;

	/** Whether this is a safe zone (no combat, allows fast travel). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	bool bIsSafeZone = false;

	/** Whether player can fast travel TO this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Zone")
	bool bAllowFastTravel = false;

	/** Pick a random encounter group weighted by SelectionWeight. */
	UFUNCTION(BlueprintPure, Category = "Zone")
	const FZoneEncounterGroup& GetRandomEncounterGroup() const;

	// UPrimaryDataAsset interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};

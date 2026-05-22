#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Inventory/ARPGItemInstance.h"
#include "Quest/ARPGQuestTypes.h"
#include "ARPGSaveGame.generated.h"

// =========================================================================
// Inventory item data (legacy v1 format)
// =========================================================================

USTRUCT(BlueprintType)
struct FARPGInventoryItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 StackMax = 99;
};

// =========================================================================
// World state — persistent world-level flags (defeated bosses, opened chests, etc.)
// =========================================================================

USTRUCT(BlueprintType)
struct FARPGWorldState
{
	GENERATED_BODY()

	/** IDs of defeated bosses (never respawn). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
	TSet<FName> DefeatedBosses;

	/** IDs of opened/looted containers (chests, barrels, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
	TSet<FName> OpenedContainers;

	/** IDs of unlocked doors/gates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
	TSet<FName> UnlockedDoors;

	/** IDs of discovered zones/areas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
	TSet<FName> DiscoveredZones;

	/** Generic persistent flags for one-off world events (cutscenes seen, levers pulled, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World")
	TSet<FName> WorldFlags;
};

// =========================================================================
// Player progression snapshot (saved to disk)
// =========================================================================

USTRUCT(BlueprintType)
struct FARPGPlayerProgression
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	FString CharacterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	int32 PlayerLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	float Experience = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	float MaxMana = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	float TotalPlayTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	FString CurrentZone;

	// --- Attribute allocation ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	int32 UnspentAttributePoints = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	int32 AllocatedStrength = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	int32 AllocatedDexterity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	int32 AllocatedIntelligence = 0;

	// --- Ability unlock state ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	int32 SkillPoints = 0;

	/** Learned abilities: RowName -> CurrentRank */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	TMap<FName, int32> LearnedAbilities;

	// --- Ability loadout ---

	/** Hotbar slot -> ability class path (FString for serialization). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Progression")
	TMap<int32, FString> AbilityLoadout;
};

// =========================================================================
// User settings (audio, graphics, controls)
// =========================================================================

USTRUCT(BlueprintType)
struct FARPGGameSettings
{
	GENERATED_BODY()

	// --- Audio ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MasterVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MusicVolume = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SFXVolume = 1.0f;

	// --- Graphics ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics")
	int32 GraphicsQuality = 3; // 0=Low, 1=Med, 2=High, 3=Ultra

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics")
	bool bVSyncEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics")
	bool bFullscreen = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Graphics")
	FIntPoint Resolution = FIntPoint(1920, 1080);

	// --- Controls ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Controls", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MouseSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Controls")
	bool bInvertY = false;
};

// =========================================================================
// Save slot metadata (lightweight header for slot list UI)
// =========================================================================

USTRUCT(BlueprintType)
struct FARPGSaveSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FString SlotName;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FString CharacterName;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 PlayerLevel = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	float PlayTimeSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FString CurrentZone;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FDateTime SaveTimestamp;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	bool bIsAutoSave = false;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	bool bExists = false;
};

// =========================================================================
// The save game object — serialized to/from disk
// =========================================================================

UCLASS()
class POF_API UARPGSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UARPGSaveGame();

	/** Save format version. Set to CurrentSaveVersion by BuildSaveGameFromCache;
	 *  defaults to 1 for deserialization of legacy v1 saves that predate this field. */
	UPROPERTY()
	int32 SaveVersion = 1;

	/** Timestamp of when the save was created. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save")
	FDateTime SaveTimestamp;

	/** Player progression data. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save")
	FARPGPlayerProgression Progression;

	/** Player inventory (legacy format — simple ID + quantity). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save")
	TArray<FARPGInventoryItem> Inventory;

	/** Rich item instance data — preserves affixes, item level, unique IDs. (v2+) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save")
	TArray<FARPGItemSaveData> ItemInstances;

	/** Which equipment slot each saved item occupies. Parallel to ItemInstances by UniqueId. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save")
	TMap<FGuid, uint8> EquippedSlots;

	/** Quest states — active, completed, and failed quests with objective progress. (v3+) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save")
	TArray<FARPGQuestState> QuestStates;

	/** Pinned quest ID for HUD tracker. (v3+) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save")
	FName PinnedQuestID;

	/** Persistent world state — defeated bosses, opened containers, unlocked doors, etc. (v3+) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save")
	FARPGWorldState WorldState;

	/** User settings (audio, graphics, controls). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save")
	FARPGGameSettings Settings;

	static constexpr int32 CurrentSaveVersion = 4;
};

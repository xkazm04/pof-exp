#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Framework/ARPGSaveGame.h"
#include "ARPGSaveSubsystem.generated.h"

class UARPGQuestSubsystem;
class UARPGInventoryComponent;
class AARPGPlayerCharacter;
class AARPGBossCharacter;

/** Broadcast after a save completes: (SlotName, bSuccess) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSaveCompleted, const FString&, SlotName, bool, bSuccess);

/** Broadcast after a load completes: (SlotName, bSuccess) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLoadCompleted, const FString&, SlotName, bool, bSuccess);

/**
 * Subsystem managing all save/load operations for the ARPG.
 * Attached to the GameInstance — lives for the entire application session.
 */
UCLASS()
class POF_API UARPGSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// =====================================================================
	// Save / Load
	// =====================================================================

	/** Save current game state to the given slot asynchronously. Broadcasts OnSaveCompleted when done. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void SaveToSlotAsync(const FString& SlotName);

	/** Save current game state to the given slot synchronously. Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool SaveToSlot(const FString& SlotName);

	/** Load game state from the given slot. Returns true on success. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool LoadFromSlot(const FString& SlotName);

	/** Delete a save slot. Returns true if the slot existed and was deleted. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool DeleteSlot(const FString& SlotName);

	/** Check if a save slot exists. */
	UFUNCTION(BlueprintPure, Category = "Save")
	bool DoesSaveExist(const FString& SlotName) const;

	// =====================================================================
	// Save Slot Management
	// =====================================================================

	/** Get the default save slot name. */
	UFUNCTION(BlueprintPure, Category = "Save")
	FString GetDefaultSlotName() const { return DefaultSlotName; }

	/** Quick save to default slot. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool QuickSave();

	/** Quick load from default slot. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool QuickLoad();

	/** Get list of all existing save slot names. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	TArray<FString> GetAllSaveSlots() const;

	/** Get metadata for a specific slot without fully loading it. Returns false if slot doesn't exist. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool GetSlotMetadata(const FString& SlotName, FDateTime& OutTimestamp, int32& OutPlayerLevel, float& OutPlayTime) const;

	// =====================================================================
	// Save Slot System (3 manual + 1 auto-save)
	// =====================================================================

	/** Number of manual save slots available. */
	static constexpr int32 NumManualSlots = 3;

	/** Get the slot name for a manual slot index (0-2). */
	UFUNCTION(BlueprintPure, Category = "Save|Slots")
	static FString GetManualSlotName(int32 SlotIndex);

	/** Get the auto-save slot name. */
	UFUNCTION(BlueprintPure, Category = "Save|Slots")
	FString GetAutoSaveSlotName() const { return AutoSaveSlotName; }

	/** Get metadata for all 4 slots (3 manual + 1 auto). */
	UFUNCTION(BlueprintCallable, Category = "Save|Slots")
	TArray<FARPGSaveSlotInfo> GetAllSlotInfo() const;

	/** Get metadata for a single slot. */
	UFUNCTION(BlueprintCallable, Category = "Save|Slots")
	FARPGSaveSlotInfo GetSlotInfo(const FString& SlotName) const;

	/** Save to a manual slot (0-2). */
	UFUNCTION(BlueprintCallable, Category = "Save|Slots")
	void SaveToManualSlot(int32 SlotIndex);

	/** Load from a manual slot (0-2). */
	UFUNCTION(BlueprintCallable, Category = "Save|Slots")
	bool LoadFromManualSlot(int32 SlotIndex);

	/** Load from the auto-save slot. */
	UFUNCTION(BlueprintCallable, Category = "Save|Slots")
	bool LoadFromAutoSave();

	// =====================================================================
	// Settings (persisted separately from game saves)
	// =====================================================================

	/** Save current settings to disk. */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	bool SaveSettings();

	/** Load settings from disk. */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	bool LoadSettings();

	/** Get the current settings (mutable reference for modification). */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	FARPGGameSettings& GetSettings() { return CachedSettings; }

	/** Apply the current graphics settings to the game. */
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ApplyGraphicsSettings();

	// =====================================================================
	// Inventory helpers (operate on cached save data)
	// =====================================================================

	/** Add an item to the cached inventory. Stacks if possible. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddInventoryItem(FName ItemID, int32 Quantity = 1);

	/** Remove an item from the cached inventory. Returns actual amount removed. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 RemoveInventoryItem(FName ItemID, int32 Quantity = 1);

	/** Check how many of an item the player has. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(FName ItemID) const;

	/** Get the full inventory array. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FARPGInventoryItem>& GetInventory() const { return CachedInventory; }

	// =====================================================================
	// Rich Item Instances (v2+)
	// =====================================================================

	/** Get the full item instances array (v2+ rich data with affixes, levels, unique IDs). */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FARPGItemSaveData>& GetItemInstances() const { return CachedItemInstances; }

	/** Get mutable item instances for modification. */
	TArray<FARPGItemSaveData>& GetMutableItemInstances() { return CachedItemInstances; }

	/** Get the equipped slot map (UniqueId → slot index). */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TMap<FGuid, uint8>& GetEquippedSlots() const { return CachedEquippedSlots; }

	/** Get mutable equipped slots for modification. */
	TMap<FGuid, uint8>& GetMutableEquippedSlots() { return CachedEquippedSlots; }

	// =====================================================================
	// World State
	// =====================================================================

	/** Get mutable world state for modification by gameplay systems. */
	UFUNCTION(BlueprintCallable, Category = "Save|World")
	FARPGWorldState& GetWorldState() { return CachedWorldState; }

	/** Mark a boss as defeated (persists across saves). */
	UFUNCTION(BlueprintCallable, Category = "Save|World")
	void MarkBossDefeated(FName BossID);

	/** Mark a container as opened/looted. */
	UFUNCTION(BlueprintCallable, Category = "Save|World")
	void MarkContainerOpened(FName ContainerID);

	/** Mark a door/gate as unlocked. */
	UFUNCTION(BlueprintCallable, Category = "Save|World")
	void MarkDoorUnlocked(FName DoorID);

	/** Mark a zone as discovered. */
	UFUNCTION(BlueprintCallable, Category = "Save|World")
	void MarkZoneDiscovered(FName ZoneID);

	/** Set a generic world flag. */
	UFUNCTION(BlueprintCallable, Category = "Save|World")
	void SetWorldFlag(FName FlagID);

	/** Check a world flag. */
	UFUNCTION(BlueprintPure, Category = "Save|World")
	bool HasWorldFlag(FName FlagID) const;

	UFUNCTION(BlueprintPure, Category = "Save|World")
	bool IsBossDefeated(FName BossID) const;

	UFUNCTION(BlueprintPure, Category = "Save|World")
	bool IsContainerOpened(FName ContainerID) const;

	// =====================================================================
	// Auto-Save
	// =====================================================================

	/** Enable/disable auto-save. */
	UFUNCTION(BlueprintCallable, Category = "Save|Auto")
	void SetAutoSaveEnabled(bool bEnabled);

	/** Whether auto-save is currently enabled. */
	UFUNCTION(BlueprintPure, Category = "Save|Auto")
	bool IsAutoSaveEnabled() const { return bAutoSaveEnabled; }

	/** Auto-save interval in seconds (default 300 = 5 minutes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Auto", meta = (ClampMin = "30"))
	float AutoSaveIntervalSeconds = 300.f;

	/** The slot name used for auto-saves. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Auto")
	FString AutoSaveSlotName = TEXT("AutoSave");

	/** Trigger an auto-save now (e.g. on zone transition). No-op if auto-save disabled. */
	UFUNCTION(BlueprintCallable, Category = "Save|Auto")
	void TriggerAutoSave();

	/** Bind auto-save to a boss defeat event. Called by encounter systems when a boss spawns. */
	UFUNCTION(BlueprintCallable, Category = "Save|Auto")
	void BindToBossDefeat(AARPGBossCharacter* Boss);

	// =====================================================================
	// Full State Gather / Restore (reads from live game systems)
	// =====================================================================

	/** Gather state from all live systems (inventory component, quest subsystem, etc.) into cache. */
	void GatherStateFromWorld();

	/** After loading, push cached state back to all live systems. */
	void RestoreStateToWorld();

	// =====================================================================
	// Delegates
	// =====================================================================

	UPROPERTY(BlueprintAssignable, Category = "Save|Events")
	FOnSaveCompleted OnSaveCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Save|Events")
	FOnLoadCompleted OnLoadCompleted;

protected:
	UPROPERTY()
	FString DefaultSlotName = TEXT("SaveSlot_0");

	UPROPERTY()
	FString SettingsSlotName = TEXT("GameSettings");

	static constexpr int32 UserIndex = 0;

	/** Maximum number of save slots allowed. */
	UPROPERTY(EditAnywhere, Category = "Save")
	int32 MaxSaveSlots = 5;

private:
	// Cached data (in-memory working copy)
	FARPGPlayerProgression CachedProgression;
	TArray<FARPGInventoryItem> CachedInventory;
	TArray<FARPGItemSaveData> CachedItemInstances;
	TMap<FGuid, uint8> CachedEquippedSlots;
	TArray<FARPGQuestState> CachedQuestStates;
	FName CachedPinnedQuestID;
	FARPGWorldState CachedWorldState;
	FARPGGameSettings CachedSettings;

	// Auto-save state
	bool bAutoSaveEnabled = true;
	bool bSaveInProgress = false;
	FTimerHandle AutoSaveTimerHandle;
	void OnAutoSaveTimer();

	/** Minimum seconds between event-triggered auto-saves (prevents rapid-fire from simultaneous events). */
	float AutoSaveThrottleCooldown = 10.f;
	double LastAutoSaveTime = 0.0;

	// Auto-save event handlers
	UFUNCTION()
	void OnPostMapChange(const FString& NewMap);

	UFUNCTION()
	void OnPlayerLevelUp(int32 NewLevel);

	UFUNCTION()
	void OnBossDefeated();

	/** Bind to player level-up delegate. Called when pawn is possessed. */
	void BindToPlayerEvents();

	/** Currently bound boss (for unbinding on cleanup). */
	TWeakObjectPtr<AARPGBossCharacter> BoundBoss;

	// Async save callback
	void OnAsyncSaveComplete(const FString& SlotName, const int32 InUserIndex, bool bSuccess);

	/** Validate a loaded save game for corruption / version mismatch. */
	bool ValidateSaveGame(const UARPGSaveGame* SaveGame) const;

	/** Run version-stepped migration chain (v1→v2→...→Current). Returns false if migration fails. */
	bool MigrateSaveGame(UARPGSaveGame* SaveGame);

	/** Populate cached data from a loaded save game object. */
	void ApplySaveToCache(const UARPGSaveGame* SaveGame);

	/** Build a save game object from cached data. */
	UARPGSaveGame* BuildSaveGameFromCache() const;

	/** Find the player's inventory component (from first local PlayerController's pawn). */
	UARPGInventoryComponent* FindPlayerInventoryComponent() const;
};

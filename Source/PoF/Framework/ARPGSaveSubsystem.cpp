#include "ARPGSaveSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameUserSettings.h"
#include "Framework/ARPGGameInstance.h"
#include "Quest/ARPGQuestSubsystem.h"
#include "Inventory/ARPGInventoryComponent.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Character/ARPGBossCharacter.h"
#include "Engine/World.h"
#include "TimerManager.h"

// =========================================================================
// Lifecycle
// =========================================================================

void UARPGSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Load settings on startup (separate from game saves)
	LoadSettings();

	// Bind to zone changes for auto-save on transition
	if (UARPGGameInstance* GI = Cast<UARPGGameInstance>(GetGameInstance()))
	{
		GI->OnPostMapChange.AddDynamic(this, &UARPGSaveSubsystem::OnPostMapChange);
	}

	// Start auto-save timer
	SetAutoSaveEnabled(bAutoSaveEnabled);

	// Deferred bind to player events (player may not exist yet at Initialize time)
	if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		FTimerHandle BindHandle;
		World->GetTimerManager().SetTimerForNextTick([WeakThis = TWeakObjectPtr<UARPGSaveSubsystem>(this)]()
		{
			if (WeakThis.IsValid())
			{
				WeakThis->BindToPlayerEvents();
			}
		});
	}

	UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Initialized (AutoSave=%s, Interval=%.0fs)"),
		bAutoSaveEnabled ? TEXT("ON") : TEXT("OFF"), AutoSaveIntervalSeconds);
}

void UARPGSaveSubsystem::Deinitialize()
{
	// Clear auto-save timer (null-safe — GameInstance/World may be torn down)
	if (UGameInstance* GIBase = GetGameInstance())
	{
		if (UWorld* World = GIBase->GetWorld())
		{
			World->GetTimerManager().ClearTimer(AutoSaveTimerHandle);
		}
	}

	// Unbind zone change delegate
	if (UARPGGameInstance* GI = Cast<UARPGGameInstance>(GetGameInstance()))
	{
		GI->OnPostMapChange.RemoveDynamic(this, &UARPGSaveSubsystem::OnPostMapChange);
	}

	// Auto-save settings on shutdown
	SaveSettings();

	UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Deinitialized — settings saved"));
	Super::Deinitialize();
}

// =========================================================================
// Save / Load
// =========================================================================

bool UARPGSaveSubsystem::SaveToSlot(const FString& SlotName)
{
	GatherStateFromWorld();

	UARPGSaveGame* SaveObj = BuildSaveGameFromCache();
	if (!SaveObj)
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] Failed to build save game object"));
		OnSaveCompleted.Broadcast(SlotName, false);
		return false;
	}

	const bool bSuccess = UGameplayStatics::SaveGameToSlot(SaveObj, SlotName, UserIndex);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Saved to slot '%s' (Lv%d, PlayTime=%.0fs)"),
			*SlotName, CachedProgression.PlayerLevel, CachedProgression.TotalPlayTime);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] Failed to save to slot '%s'"), *SlotName);
	}

	OnSaveCompleted.Broadcast(SlotName, bSuccess);
	return bSuccess;
}

void UARPGSaveSubsystem::SaveToSlotAsync(const FString& SlotName)
{
	if (bSaveInProgress)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSubsystem] Async save already in progress — skipping '%s'"), *SlotName);
		return;
	}

	GatherStateFromWorld();

	UARPGSaveGame* SaveObj = BuildSaveGameFromCache();
	if (!SaveObj)
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] Failed to build save game object for async save"));
		OnSaveCompleted.Broadcast(SlotName, false);
		return;
	}

	bSaveInProgress = true;
	UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Async saving to slot '%s'..."), *SlotName);

	FAsyncSaveGameToSlotDelegate AsyncDelegate;
	AsyncDelegate.BindUObject(this, &UARPGSaveSubsystem::OnAsyncSaveComplete);
	UGameplayStatics::AsyncSaveGameToSlot(SaveObj, SlotName, UserIndex, AsyncDelegate);
}

void UARPGSaveSubsystem::OnAsyncSaveComplete(const FString& SlotName, const int32 InUserIndex, bool bSuccess)
{
	bSaveInProgress = false;

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Async save to '%s' completed successfully"), *SlotName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] Async save to '%s' failed"), *SlotName);
	}

	OnSaveCompleted.Broadcast(SlotName, bSuccess);
}

bool UARPGSaveSubsystem::LoadFromSlot(const FString& SlotName)
{
	if (!DoesSaveExist(SlotName))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSubsystem] Slot '%s' does not exist"), *SlotName);
		OnLoadCompleted.Broadcast(SlotName, false);
		return false;
	}

	USaveGame* RawSave = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
	UARPGSaveGame* SaveObj = Cast<UARPGSaveGame>(RawSave);

	if (!SaveObj)
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] Slot '%s' — failed to cast to UARPGSaveGame (corrupted?)"), *SlotName);
		OnLoadCompleted.Broadcast(SlotName, false);
		return false;
	}

	if (!ValidateSaveGame(SaveObj))
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] Slot '%s' — save validation failed (version mismatch or corruption)"), *SlotName);
		OnLoadCompleted.Broadcast(SlotName, false);
		return false;
	}

	// Run version-stepped migration before applying to cache
	if (!MigrateSaveGame(SaveObj))
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] Slot '%s' — migration failed from version %d"), *SlotName, SaveObj->SaveVersion);
		OnLoadCompleted.Broadcast(SlotName, false);
		return false;
	}

	ApplySaveToCache(SaveObj);

	// Push loaded data back to all live systems
	RestoreStateToWorld();

	UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Loaded slot '%s' (Lv%d, PlayTime=%.0fs, Quests=%d, WorldFlags=%d)"),
		*SlotName, CachedProgression.PlayerLevel, CachedProgression.TotalPlayTime,
		CachedQuestStates.Num(), CachedWorldState.WorldFlags.Num());

	OnLoadCompleted.Broadcast(SlotName, true);
	return true;
}

bool UARPGSaveSubsystem::DeleteSlot(const FString& SlotName)
{
	if (!DoesSaveExist(SlotName)) return false;

	const bool bDeleted = UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
	if (bDeleted)
	{
		UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Deleted slot '%s'"), *SlotName);
	}
	return bDeleted;
}

bool UARPGSaveSubsystem::DoesSaveExist(const FString& SlotName) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

// =========================================================================
// Save Slot Management
// =========================================================================

bool UARPGSaveSubsystem::QuickSave()
{
	return SaveToSlot(DefaultSlotName);
}

bool UARPGSaveSubsystem::QuickLoad()
{
	return LoadFromSlot(DefaultSlotName);
}

TArray<FString> UARPGSaveSubsystem::GetAllSaveSlots() const
{
	TArray<FString> Slots;
	for (int32 i = 0; i < NumManualSlots; ++i)
	{
		FString SlotName = GetManualSlotName(i);
		if (DoesSaveExist(SlotName))
		{
			Slots.Add(SlotName);
		}
	}
	if (DoesSaveExist(AutoSaveSlotName))
	{
		Slots.Add(AutoSaveSlotName);
	}
	return Slots;
}

// =========================================================================
// Save Slot System (3 manual + 1 auto-save)
// =========================================================================

FString UARPGSaveSubsystem::GetManualSlotName(int32 SlotIndex)
{
	return FString::Printf(TEXT("SaveSlot_%d"), FMath::Clamp(SlotIndex, 0, NumManualSlots - 1));
}

FARPGSaveSlotInfo UARPGSaveSubsystem::GetSlotInfo(const FString& SlotName) const
{
	FARPGSaveSlotInfo Info;
	Info.SlotName = SlotName;
	Info.bIsAutoSave = (SlotName == AutoSaveSlotName);

	if (!DoesSaveExist(SlotName))
	{
		Info.bExists = false;
		return Info;
	}

	USaveGame* RawSave = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
	const UARPGSaveGame* SaveObj = Cast<UARPGSaveGame>(RawSave);
	if (!SaveObj)
	{
		Info.bExists = false;
		return Info;
	}

	Info.bExists = true;
	Info.CharacterName = SaveObj->Progression.CharacterName;
	Info.PlayerLevel = SaveObj->Progression.PlayerLevel;
	Info.PlayTimeSeconds = SaveObj->Progression.TotalPlayTime;
	Info.CurrentZone = SaveObj->Progression.CurrentZone;
	Info.SaveTimestamp = SaveObj->SaveTimestamp;
	return Info;
}

TArray<FARPGSaveSlotInfo> UARPGSaveSubsystem::GetAllSlotInfo() const
{
	TArray<FARPGSaveSlotInfo> AllSlots;
	AllSlots.Reserve(NumManualSlots + 1);

	for (int32 i = 0; i < NumManualSlots; ++i)
	{
		AllSlots.Add(GetSlotInfo(GetManualSlotName(i)));
	}
	AllSlots.Add(GetSlotInfo(AutoSaveSlotName));

	return AllSlots;
}

void UARPGSaveSubsystem::SaveToManualSlot(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= NumManualSlots)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSubsystem] Invalid manual slot index %d (max %d)"), SlotIndex, NumManualSlots - 1);
		return;
	}
	SaveToSlotAsync(GetManualSlotName(SlotIndex));
}

bool UARPGSaveSubsystem::LoadFromManualSlot(int32 SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= NumManualSlots)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSubsystem] Invalid manual slot index %d (max %d)"), SlotIndex, NumManualSlots - 1);
		return false;
	}
	return LoadFromSlot(GetManualSlotName(SlotIndex));
}

bool UARPGSaveSubsystem::LoadFromAutoSave()
{
	return LoadFromSlot(AutoSaveSlotName);
}

bool UARPGSaveSubsystem::GetSlotMetadata(const FString& SlotName, FDateTime& OutTimestamp, int32& OutPlayerLevel, float& OutPlayTime) const
{
	if (!DoesSaveExist(SlotName)) return false;

	USaveGame* RawSave = UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex);
	const UARPGSaveGame* SaveObj = Cast<UARPGSaveGame>(RawSave);
	if (!SaveObj) return false;

	OutTimestamp = SaveObj->SaveTimestamp;
	OutPlayerLevel = SaveObj->Progression.PlayerLevel;
	OutPlayTime = SaveObj->Progression.TotalPlayTime;
	return true;
}

// =========================================================================
// Settings
// =========================================================================

bool UARPGSaveSubsystem::SaveSettings()
{
	UARPGSaveGame* SettingsSave = NewObject<UARPGSaveGame>();
	SettingsSave->Settings = CachedSettings;
	SettingsSave->SaveTimestamp = FDateTime::Now();

	const bool bSuccess = UGameplayStatics::SaveGameToSlot(SettingsSave, SettingsSlotName, UserIndex);
	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Settings saved"));
	}
	return bSuccess;
}

bool UARPGSaveSubsystem::LoadSettings()
{
	if (!UGameplayStatics::DoesSaveGameExist(SettingsSlotName, UserIndex))
	{
		UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] No settings file found — using defaults"));
		return false;
	}

	USaveGame* RawSave = UGameplayStatics::LoadGameFromSlot(SettingsSlotName, UserIndex);
	const UARPGSaveGame* SettingsSave = Cast<UARPGSaveGame>(RawSave);
	if (!SettingsSave)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSubsystem] Settings file corrupted — using defaults"));
		return false;
	}

	CachedSettings = SettingsSave->Settings;
	ApplyGraphicsSettings();
	UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Settings loaded"));
	return true;
}

void UARPGSaveSubsystem::ApplyGraphicsSettings()
{
	UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!UserSettings) return;

	UserSettings->SetScreenResolution(CachedSettings.Resolution);
	UserSettings->SetVSyncEnabled(CachedSettings.bVSyncEnabled);
	UserSettings->SetFullscreenMode(CachedSettings.bFullscreen ? EWindowMode::Fullscreen : EWindowMode::Windowed);
	UserSettings->SetOverallScalabilityLevel(CachedSettings.GraphicsQuality);
	UserSettings->ApplySettings(false);

	UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Graphics settings applied (Quality=%d, VSync=%d, Fullscreen=%d)"),
		CachedSettings.GraphicsQuality, CachedSettings.bVSyncEnabled, CachedSettings.bFullscreen);
}

// =========================================================================
// Inventory
// =========================================================================

bool UARPGSaveSubsystem::AddInventoryItem(FName ItemID, int32 Quantity)
{
	if (ItemID.IsNone() || Quantity <= 0) return false;

	// Try to stack onto existing entry
	for (FARPGInventoryItem& Item : CachedInventory)
	{
		if (Item.ItemID == ItemID)
		{
			const int32 CanAdd = Item.StackMax - Item.Quantity;
			if (CanAdd >= Quantity)
			{
				Item.Quantity += Quantity;
				return true;
			}
			else if (CanAdd > 0)
			{
				Item.Quantity = Item.StackMax;
				Quantity -= CanAdd;
				// Remaining quantity falls through to create a new stack
			}
		}
	}

	// Create new stack(s) for remaining quantity
	while (Quantity > 0)
	{
		FARPGInventoryItem NewItem;
		NewItem.ItemID = ItemID;
		NewItem.Quantity = FMath::Min(Quantity, NewItem.StackMax);
		Quantity -= NewItem.Quantity;
		CachedInventory.Add(NewItem);
	}

	return true;
}

int32 UARPGSaveSubsystem::RemoveInventoryItem(FName ItemID, int32 Quantity)
{
	if (ItemID.IsNone() || Quantity <= 0) return 0;

	int32 Removed = 0;

	for (int32 i = CachedInventory.Num() - 1; i >= 0 && Quantity > 0; --i)
	{
		FARPGInventoryItem& Item = CachedInventory[i];
		if (Item.ItemID != ItemID) continue;

		const int32 ToRemove = FMath::Min(Item.Quantity, Quantity);
		Item.Quantity -= ToRemove;
		Quantity -= ToRemove;
		Removed += ToRemove;

		if (Item.Quantity <= 0)
		{
			CachedInventory.RemoveAt(i);
		}
	}

	return Removed;
}

int32 UARPGSaveSubsystem::GetItemCount(FName ItemID) const
{
	int32 Total = 0;
	for (const FARPGInventoryItem& Item : CachedInventory)
	{
		if (Item.ItemID == ItemID)
		{
			Total += Item.Quantity;
		}
	}
	return Total;
}

// =========================================================================
// Internal helpers
// =========================================================================

bool UARPGSaveSubsystem::MigrateSaveGame(UARPGSaveGame* SaveGame)
{
	if (!SaveGame) return false;

	const int32 OriginalVersion = SaveGame->SaveVersion;

	// Step through each version upgrade sequentially
	while (SaveGame->SaveVersion < UARPGSaveGame::CurrentSaveVersion)
	{
		switch (SaveGame->SaveVersion)
		{
		case 1:
		{
			// v1 → v2: Convert legacy Inventory (FARPGInventoryItem) to ItemInstances (FARPGItemSaveData)
			UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Migrating save v1 → v2: converting %d Inventory entries to ItemInstances"),
				SaveGame->Inventory.Num());

			for (const FARPGInventoryItem& LegacyItem : SaveGame->Inventory)
			{
				FARPGItemSaveData NewItem;
				// Map legacy ItemID → PrimaryAssetId (e.g. "ARPGItemDefinition:DA_<ItemID>")
				NewItem.DefinitionId = FPrimaryAssetId(
					FPrimaryAssetType(TEXT("ARPGItemDefinition")),
					*FString::Printf(TEXT("DA_%s"), *LegacyItem.ItemID.ToString()));
				NewItem.StackCount = LegacyItem.Quantity;
				NewItem.ItemLevel = 1;
				NewItem.UniqueId = FGuid::NewGuid();
				// Affixes left empty — legacy items had no affix data
				SaveGame->ItemInstances.Add(NewItem);
			}

			// Clear legacy inventory after conversion
			SaveGame->Inventory.Empty();

			// Clear EquippedSlots — legacy GUIDs are now invalid after reassignment
			if (SaveGame->EquippedSlots.Num() > 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("[SaveSubsystem] v1→v2: clearing %d orphaned EquippedSlots (GUIDs reassigned)"),
					SaveGame->EquippedSlots.Num());
				SaveGame->EquippedSlots.Empty();
			}

			SaveGame->SaveVersion = 2;
			break;
		}
		case 2:
		{
			// v2 → v3: Add quest states, pinned quest, and world state (all default-empty)
			UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Migrating save v2 → v3: adding quest and world state fields"));
			SaveGame->SaveVersion = 3;
			break;
		}
		case 3:
		{
			// v3 → v4: Add CharacterName to progression (default empty)
			UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Migrating save v3 → v4: adding CharacterName field"));
			// CharacterName is default-empty. Existing saves get a placeholder.
			if (SaveGame->Progression.CharacterName.IsEmpty())
			{
				SaveGame->Progression.CharacterName = TEXT("Hero");
			}
			SaveGame->SaveVersion = 4;
			break;
		}
		default:
			UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] No migration path from save version %d"), SaveGame->SaveVersion);
			return false;
		}
	}

	if (OriginalVersion != SaveGame->SaveVersion)
	{
		UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Migration complete: v%d → v%d"), OriginalVersion, SaveGame->SaveVersion);
	}

	return true;
}

bool UARPGSaveSubsystem::ValidateSaveGame(const UARPGSaveGame* SaveGame) const
{
	if (!SaveGame) return false;

	// Version check — reject saves from incompatible future versions
	if (SaveGame->SaveVersion > UARPGSaveGame::CurrentSaveVersion)
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] Save version %d > current %d — cannot load"),
			SaveGame->SaveVersion, UARPGSaveGame::CurrentSaveVersion);
		return false;
	}

	// Basic integrity: level must be positive
	if (SaveGame->Progression.PlayerLevel < 1)
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] Invalid PlayerLevel %d in save data"),
			SaveGame->Progression.PlayerLevel);
		return false;
	}

	// Health/Mana sanity
	if (SaveGame->Progression.MaxHealth <= 0.f || SaveGame->Progression.MaxMana < 0.f)
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] Invalid MaxHealth(%.1f) or MaxMana(%.1f) in save data"),
			SaveGame->Progression.MaxHealth, SaveGame->Progression.MaxMana);
		return false;
	}

	// Play time sanity (negative is invalid, but very large values are fine)
	if (SaveGame->Progression.TotalPlayTime < 0.f)
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] Negative TotalPlayTime in save data"));
		return false;
	}

	// EquippedSlots referential integrity — every GUID must exist in ItemInstances
	if (SaveGame->EquippedSlots.Num() > 0)
	{
		// Build a set of valid UniqueIds from ItemInstances for O(n) lookup
		TSet<FGuid> ValidItemIds;
		ValidItemIds.Reserve(SaveGame->ItemInstances.Num());
		for (const FARPGItemSaveData& Item : SaveGame->ItemInstances)
		{
			ValidItemIds.Add(Item.UniqueId);
		}

		int32 OrphanCount = 0;
		for (const auto& Pair : SaveGame->EquippedSlots)
		{
			if (!ValidItemIds.Contains(Pair.Key))
			{
				++OrphanCount;
				UE_LOG(LogTemp, Warning, TEXT("[SaveSubsystem] EquippedSlots contains orphaned GUID %s (slot %d) — no matching ItemInstance"),
					*Pair.Key.ToString(), Pair.Value);
			}
		}

		if (OrphanCount > 0)
		{
			UE_LOG(LogTemp, Error, TEXT("[SaveSubsystem] %d/%d EquippedSlot entries reference non-existent ItemInstances"),
				OrphanCount, SaveGame->EquippedSlots.Num());
			return false;
		}
	}

	return true;
}

void UARPGSaveSubsystem::ApplySaveToCache(const UARPGSaveGame* SaveGame)
{
	if (!SaveGame) return;

	CachedProgression = SaveGame->Progression;
	CachedInventory = SaveGame->Inventory;
	CachedItemInstances = SaveGame->ItemInstances;
	CachedEquippedSlots = SaveGame->EquippedSlots;
	CachedQuestStates = SaveGame->QuestStates;
	CachedPinnedQuestID = SaveGame->PinnedQuestID;
	CachedWorldState = SaveGame->WorldState;
	CachedSettings = SaveGame->Settings;
}

UARPGSaveGame* UARPGSaveSubsystem::BuildSaveGameFromCache() const
{
	UARPGSaveGame* SaveObj = NewObject<UARPGSaveGame>();
	SaveObj->SaveVersion = UARPGSaveGame::CurrentSaveVersion;
	SaveObj->SaveTimestamp = FDateTime::Now();
	SaveObj->Progression = CachedProgression;
	SaveObj->Inventory = CachedInventory;
	SaveObj->ItemInstances = CachedItemInstances;
	SaveObj->EquippedSlots = CachedEquippedSlots;
	SaveObj->QuestStates = CachedQuestStates;
	SaveObj->PinnedQuestID = CachedPinnedQuestID;
	SaveObj->WorldState = CachedWorldState;
	SaveObj->Settings = CachedSettings;
	return SaveObj;
}

// =========================================================================
// World State
// =========================================================================

void UARPGSaveSubsystem::MarkBossDefeated(FName BossID)
{
	CachedWorldState.DefeatedBosses.Add(BossID);
}

void UARPGSaveSubsystem::MarkContainerOpened(FName ContainerID)
{
	CachedWorldState.OpenedContainers.Add(ContainerID);
}

void UARPGSaveSubsystem::MarkDoorUnlocked(FName DoorID)
{
	CachedWorldState.UnlockedDoors.Add(DoorID);
}

void UARPGSaveSubsystem::MarkZoneDiscovered(FName ZoneID)
{
	CachedWorldState.DiscoveredZones.Add(ZoneID);
}

void UARPGSaveSubsystem::SetWorldFlag(FName FlagID)
{
	CachedWorldState.WorldFlags.Add(FlagID);
}

bool UARPGSaveSubsystem::HasWorldFlag(FName FlagID) const
{
	return CachedWorldState.WorldFlags.Contains(FlagID);
}

bool UARPGSaveSubsystem::IsBossDefeated(FName BossID) const
{
	return CachedWorldState.DefeatedBosses.Contains(BossID);
}

bool UARPGSaveSubsystem::IsContainerOpened(FName ContainerID) const
{
	return CachedWorldState.OpenedContainers.Contains(ContainerID);
}

// =========================================================================
// Auto-Save
// =========================================================================

void UARPGSaveSubsystem::SetAutoSaveEnabled(bool bEnabled)
{
	bAutoSaveEnabled = bEnabled;

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World) return;

	World->GetTimerManager().ClearTimer(AutoSaveTimerHandle);

	if (bAutoSaveEnabled)
	{
		World->GetTimerManager().SetTimer(AutoSaveTimerHandle, this,
			&UARPGSaveSubsystem::OnAutoSaveTimer, AutoSaveIntervalSeconds, true);
	}
}

void UARPGSaveSubsystem::OnAutoSaveTimer()
{
	TriggerAutoSave();
}

void UARPGSaveSubsystem::TriggerAutoSave()
{
	if (!bAutoSaveEnabled) return;

	// Throttle: skip if another auto-save happened within cooldown window
	const double Now = FPlatformTime::Seconds();
	if (Now - LastAutoSaveTime < AutoSaveThrottleCooldown)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[SaveSubsystem] Auto-save throttled (%.1fs since last)"),
			Now - LastAutoSaveTime);
		return;
	}
	LastAutoSaveTime = Now;

	UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Auto-saving to '%s'..."), *AutoSaveSlotName);
	SaveToSlotAsync(AutoSaveSlotName);
}

void UARPGSaveSubsystem::OnPostMapChange(const FString& NewMap)
{
	if (bAutoSaveEnabled)
	{
		MarkZoneDiscovered(FName(*NewMap));
		UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Zone transition to '%s' — triggering auto-save"), *NewMap);
		TriggerAutoSave();
	}
}

void UARPGSaveSubsystem::OnPlayerLevelUp(int32 NewLevel)
{
	UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Player leveled up to %d — triggering auto-save"), NewLevel);
	TriggerAutoSave();
}

void UARPGSaveSubsystem::OnBossDefeated()
{
	UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Boss defeated — triggering auto-save"));
	TriggerAutoSave();
}

void UARPGSaveSubsystem::BindToPlayerEvents()
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return;

	AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(PC->GetPawn());
	if (!Player) return;

	Player->OnPlayerLevelUp.AddUniqueDynamic(this, &UARPGSaveSubsystem::OnPlayerLevelUp);
	UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Bound to player level-up events"));
}

void UARPGSaveSubsystem::BindToBossDefeat(AARPGBossCharacter* Boss)
{
	if (!Boss) return;

	// Unbind from previous boss if any
	if (BoundBoss.IsValid())
	{
		BoundBoss->OnBossDefeated.RemoveDynamic(this, &UARPGSaveSubsystem::OnBossDefeated);
	}

	Boss->OnBossDefeated.AddUniqueDynamic(this, &UARPGSaveSubsystem::OnBossDefeated);
	BoundBoss = Boss;
	UE_LOG(LogTemp, Log, TEXT("[SaveSubsystem] Bound to boss defeat event for '%s'"), *Boss->GetBossName().ToString());
}

// =========================================================================
// Full State Gather / Restore
// =========================================================================

void UARPGSaveSubsystem::GatherStateFromWorld()
{
	// 1. Progression from GameInstance
	if (const UARPGGameInstance* GI = Cast<UARPGGameInstance>(GetGameInstance()))
	{
		CachedProgression.PlayerLevel = GI->PlayerLevel;
		CachedProgression.Experience = GI->Experience;
		CachedProgression.MaxHealth = GI->MaxHealth;
		CachedProgression.MaxMana = GI->MaxMana;
		CachedProgression.CurrentZone = GI->CurrentZone;
		CachedProgression.TotalPlayTime = GI->TotalPlayTime;

		// Preserve character name if already set; default for new saves
		if (CachedProgression.CharacterName.IsEmpty())
		{
			CachedProgression.CharacterName = TEXT("Hero");
		}
	}

	// 2. Inventory from the player's InventoryComponent
	if (UARPGInventoryComponent* InvComp = FindPlayerInventoryComponent())
	{
		CachedItemInstances.Empty();
		CachedEquippedSlots.Empty();
		InvComp->GatherSaveData(CachedItemInstances, CachedEquippedSlots);
	}

	// 3. Quest state from QuestSubsystem
	if (UARPGQuestSubsystem* QuestSub = GetGameInstance()->GetSubsystem<UARPGQuestSubsystem>())
	{
		CachedQuestStates = QuestSub->GetAllQuestStates();
		CachedPinnedQuestID = QuestSub->GetPinnedQuestID();
	}

	// 4. World state is already in cache (modified by MarkBossDefeated, etc.)
}

void UARPGSaveSubsystem::RestoreStateToWorld()
{
	// 1. Push progression to GameInstance
	if (UARPGGameInstance* GI = Cast<UARPGGameInstance>(GetGameInstance()))
	{
		GI->SyncProgressionFromSubsystem(CachedProgression);
	}

	// 2. Restore quest state
	if (UARPGQuestSubsystem* QuestSub = GetGameInstance()->GetSubsystem<UARPGQuestSubsystem>())
	{
		QuestSub->RestoreQuestStates(CachedQuestStates, CachedPinnedQuestID);
	}

	// 3. Inventory restoration is deferred — InventoryComponent calls RestoreFromSaveData
	//    after the pawn is possessed and the component is ready (BeginPlay timing).
	//    The subsystem exposes GetItemInstances()/GetEquippedSlots() for that purpose.
}

UARPGInventoryComponent* UARPGSaveSubsystem::FindPlayerInventoryComponent() const
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World) return nullptr;

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC) return nullptr;

	APawn* Pawn = PC->GetPawn();
	if (!Pawn) return nullptr;

	return Pawn->FindComponentByClass<UARPGInventoryComponent>();
}

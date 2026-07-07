#include "Test/Framework/VSSaveLoadTest.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Framework/ARPGSaveGame.h"
#include "Kismet/GameplayStatics.h"

/**
 * Save Points Test Gate — save/load round-trip integrity.
 *
 * Pure logic gate (no PIE/world/map needed — safe under shared-tree
 * concurrency). Builds a UARPGSaveGame populated with representative data in
 * every persisted section (version, timestamp, progression + attribute
 * allocation + ability maps, legacy inventory, equipped-slot map, pinned
 * quest, world state sets, settings), serializes it through
 * UGameplayStatics::SaveGameToMemory, deserializes with LoadGameFromMemory,
 * and TestEquals every written field on the reloaded object. Also asserts the
 * anti-corruption seams: the save carries CurrentSaveVersion (so the
 * migration chain can key off it), a fresh object defaults to legacy v1, and
 * loading an empty buffer yields nullptr instead of a garbage object.
 *
 * Runs headless via:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests Project.Functional Tests.PoF.SavePoints;Quit" -unattended -nopause -nullrhi -log
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSSaveLoadTest,
	"Project.Functional Tests.PoF.SavePoints.RoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSSaveLoadTest::RunTest(const FString& /*Parameters*/)
{
	UARPGSaveGame* Save = NewObject<UARPGSaveGame>(GetTransientPackage());
	if (!TestNotNull(TEXT("Save game object created"), Save))
	{
		return false;
	}

	// 1. Legacy default: a save that predates the version field deserializes as v1.
	TestEqual(TEXT("Fresh save defaults to legacy version 1"), Save->SaveVersion, 1);

	// --- Populate every persisted section with distinctive, non-default values ---

	Save->SaveVersion = UARPGSaveGame::CurrentSaveVersion;
	Save->SaveTimestamp = FDateTime(2026, 7, 7, 12, 30, 45);

	// Progression (core stats + zone).
	Save->Progression.CharacterName = TEXT("Roundtrip Rogue");
	Save->Progression.PlayerLevel = 17;
	Save->Progression.Experience = 4312.5f;
	Save->Progression.MaxHealth = 235.f;
	Save->Progression.MaxMana = 90.f;
	Save->Progression.TotalPlayTime = 7261.f;
	Save->Progression.CurrentZone = TEXT("Arena_Ancient");

	// Attribute allocation.
	Save->Progression.UnspentAttributePoints = 3;
	Save->Progression.AllocatedStrength = 5;
	Save->Progression.AllocatedDexterity = 9;
	Save->Progression.AllocatedIntelligence = 2;

	// Ability unlock state + hotbar loadout.
	Save->Progression.SkillPoints = 4;
	Save->Progression.LearnedAbilities.Add(TEXT("Fireball"), 3);
	Save->Progression.LearnedAbilities.Add(TEXT("FrostNova"), 1);
	Save->Progression.AbilityLoadout.Add(0, TEXT("/Game/Abilities/GA_Fireball.GA_Fireball_C"));
	Save->Progression.AbilityLoadout.Add(2, TEXT("/Game/Abilities/GA_FrostNova.GA_FrostNova_C"));

	// Legacy inventory (ID + quantity + stack size).
	FARPGInventoryItem Potion;
	Potion.ItemID = TEXT("HealthPotion");
	Potion.Quantity = 7;
	Potion.StackMax = 20;
	Save->Inventory.Add(Potion);

	FARPGInventoryItem Ore;
	Ore.ItemID = TEXT("IronOre");
	Ore.Quantity = 42;
	Ore.StackMax = 99;
	Save->Inventory.Add(Ore);

	// Equipped-slot map (UniqueId -> slot index).
	const FGuid WeaponGuid = FGuid(0x12345678, 0x9ABCDEF0, 0x0FEDCBA9, 0x87654321);
	Save->EquippedSlots.Add(WeaponGuid, 1);

	// Quest tracker pin.
	Save->PinnedQuestID = TEXT("Quest_ClearArena");

	// World state — one entry per persistence set.
	Save->WorldState.DefeatedBosses.Add(TEXT("Boss_SithLord"));
	Save->WorldState.OpenedContainers.Add(TEXT("Chest_Arena_01"));
	Save->WorldState.UnlockedDoors.Add(TEXT("Door_Vault"));
	Save->WorldState.DiscoveredZones.Add(TEXT("Zone_Catacombs"));
	Save->WorldState.WorldFlags.Add(TEXT("Cutscene_Intro_Seen"));

	// Settings (audio/graphics/controls).
	Save->Settings.MasterVolume = 0.65f;
	Save->Settings.MusicVolume = 0.4f;
	Save->Settings.SFXVolume = 0.9f;
	Save->Settings.GraphicsQuality = 1;
	Save->Settings.bVSyncEnabled = false;
	Save->Settings.bFullscreen = false;
	Save->Settings.Resolution = FIntPoint(2560, 1440);
	Save->Settings.MouseSensitivity = 2.25f;
	Save->Settings.bInvertY = true;

	// 2. Serialize to an in-memory buffer (same path SaveGameToSlot uses).
	TArray<uint8> Buffer;
	TestTrue(TEXT("SaveGameToMemory succeeds"), UGameplayStatics::SaveGameToMemory(Save, Buffer));
	TestTrue(TEXT("Serialized buffer is non-empty"), Buffer.Num() > 0);

	// 3. Reload from the buffer into a brand-new object.
	UARPGSaveGame* Loaded = Cast<UARPGSaveGame>(UGameplayStatics::LoadGameFromMemory(Buffer));
	if (!TestNotNull(TEXT("LoadGameFromMemory returns a UARPGSaveGame"), Loaded))
	{
		return false;
	}
	TestTrue(TEXT("Loaded object is a distinct instance"), Loaded != Save);

	// 4. Version + timestamp survive (migration chain keys off SaveVersion).
	TestEqual(TEXT("SaveVersion round-trips as CurrentSaveVersion"), Loaded->SaveVersion, UARPGSaveGame::CurrentSaveVersion);
	TestEqual(TEXT("SaveTimestamp round-trips"), Loaded->SaveTimestamp, FDateTime(2026, 7, 7, 12, 30, 45));

	// 5. Progression round-trips field-for-field.
	TestEqual(TEXT("CharacterName round-trips"), Loaded->Progression.CharacterName, FString(TEXT("Roundtrip Rogue")));
	TestEqual(TEXT("PlayerLevel round-trips"), Loaded->Progression.PlayerLevel, 17);
	TestEqual(TEXT("Experience round-trips"), Loaded->Progression.Experience, 4312.5f);
	TestEqual(TEXT("MaxHealth round-trips"), Loaded->Progression.MaxHealth, 235.f);
	TestEqual(TEXT("MaxMana round-trips"), Loaded->Progression.MaxMana, 90.f);
	TestEqual(TEXT("TotalPlayTime round-trips"), Loaded->Progression.TotalPlayTime, 7261.f);
	TestEqual(TEXT("CurrentZone round-trips"), Loaded->Progression.CurrentZone, FString(TEXT("Arena_Ancient")));

	TestEqual(TEXT("UnspentAttributePoints round-trips"), Loaded->Progression.UnspentAttributePoints, 3);
	TestEqual(TEXT("AllocatedStrength round-trips"), Loaded->Progression.AllocatedStrength, 5);
	TestEqual(TEXT("AllocatedDexterity round-trips"), Loaded->Progression.AllocatedDexterity, 9);
	TestEqual(TEXT("AllocatedIntelligence round-trips"), Loaded->Progression.AllocatedIntelligence, 2);

	TestEqual(TEXT("SkillPoints round-trips"), Loaded->Progression.SkillPoints, 4);
	TestEqual(TEXT("LearnedAbilities count round-trips"), Loaded->Progression.LearnedAbilities.Num(), 2);
	TestEqual(TEXT("Fireball rank round-trips"), Loaded->Progression.LearnedAbilities.FindRef(TEXT("Fireball")), 3);
	TestEqual(TEXT("FrostNova rank round-trips"), Loaded->Progression.LearnedAbilities.FindRef(TEXT("FrostNova")), 1);
	TestEqual(TEXT("AbilityLoadout count round-trips"), Loaded->Progression.AbilityLoadout.Num(), 2);
	TestEqual(
		TEXT("Hotbar slot 0 class path round-trips"),
		Loaded->Progression.AbilityLoadout.FindRef(0),
		FString(TEXT("/Game/Abilities/GA_Fireball.GA_Fireball_C")));
	TestEqual(
		TEXT("Hotbar slot 2 class path round-trips"),
		Loaded->Progression.AbilityLoadout.FindRef(2),
		FString(TEXT("/Game/Abilities/GA_FrostNova.GA_FrostNova_C")));

	// 6. Inventory round-trips in order with quantities intact.
	if (TestEqual(TEXT("Inventory item count round-trips"), Loaded->Inventory.Num(), 2))
	{
		TestEqual(TEXT("Inventory[0] ItemID round-trips"), Loaded->Inventory[0].ItemID, FName(TEXT("HealthPotion")));
		TestEqual(TEXT("Inventory[0] Quantity round-trips"), Loaded->Inventory[0].Quantity, 7);
		TestEqual(TEXT("Inventory[0] StackMax round-trips"), Loaded->Inventory[0].StackMax, 20);
		TestEqual(TEXT("Inventory[1] ItemID round-trips"), Loaded->Inventory[1].ItemID, FName(TEXT("IronOre")));
		TestEqual(TEXT("Inventory[1] Quantity round-trips"), Loaded->Inventory[1].Quantity, 42);
	}

	// 7. Equipped-slot map round-trips (FGuid key + slot value).
	TestEqual(TEXT("EquippedSlots count round-trips"), Loaded->EquippedSlots.Num(), 1);
	const uint8* LoadedSlot = Loaded->EquippedSlots.Find(WeaponGuid);
	if (TestNotNull(TEXT("Equipped weapon GUID key survives"), LoadedSlot))
	{
		TestEqual(TEXT("Equipped weapon slot index round-trips"), static_cast<int32>(*LoadedSlot), 1);
	}

	// 8. Quest pin + world state sets round-trip.
	TestEqual(TEXT("PinnedQuestID round-trips"), Loaded->PinnedQuestID, FName(TEXT("Quest_ClearArena")));
	TestTrue(TEXT("Defeated boss survives"), Loaded->WorldState.DefeatedBosses.Contains(TEXT("Boss_SithLord")));
	TestTrue(TEXT("Opened container survives"), Loaded->WorldState.OpenedContainers.Contains(TEXT("Chest_Arena_01")));
	TestTrue(TEXT("Unlocked door survives"), Loaded->WorldState.UnlockedDoors.Contains(TEXT("Door_Vault")));
	TestTrue(TEXT("Discovered zone survives"), Loaded->WorldState.DiscoveredZones.Contains(TEXT("Zone_Catacombs")));
	TestTrue(TEXT("World flag survives"), Loaded->WorldState.WorldFlags.Contains(TEXT("Cutscene_Intro_Seen")));
	TestEqual(TEXT("No phantom bosses appear"), Loaded->WorldState.DefeatedBosses.Num(), 1);

	// 9. Settings round-trip (non-default values in every section).
	TestEqual(TEXT("MasterVolume round-trips"), Loaded->Settings.MasterVolume, 0.65f);
	TestEqual(TEXT("MusicVolume round-trips"), Loaded->Settings.MusicVolume, 0.4f);
	TestEqual(TEXT("SFXVolume round-trips"), Loaded->Settings.SFXVolume, 0.9f);
	TestEqual(TEXT("GraphicsQuality round-trips"), Loaded->Settings.GraphicsQuality, 1);
	TestFalse(TEXT("bVSyncEnabled round-trips as false"), Loaded->Settings.bVSyncEnabled);
	TestFalse(TEXT("bFullscreen round-trips as false"), Loaded->Settings.bFullscreen);
	TestEqual(TEXT("Resolution round-trips"), Loaded->Settings.Resolution, FIntPoint(2560, 1440));
	TestEqual(TEXT("MouseSensitivity round-trips"), Loaded->Settings.MouseSensitivity, 2.25f);
	TestTrue(TEXT("bInvertY round-trips as true"), Loaded->Settings.bInvertY);

	// 10. Anti-corruption: an empty buffer must yield nullptr, never a garbage save.
	TArray<uint8> EmptyBuffer;
	TestNull(TEXT("Loading an empty buffer returns nullptr"), UGameplayStatics::LoadGameFromMemory(EmptyBuffer));

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Dialogue/ARPGNPCActor.h"
#include "AbilitySystem/ARPGAttributeInitData.h"
#include "UObject/Package.h"

/**
 * Character catalog gate for the named quest-giver NPC "Captain Vael"
 * (Catalog Pipeline, Game-Assets / Character row; mirrors the Fireball gate).
 *
 * Asserts that the existing AARPGNPCActor production type can carry Captain
 * Vael's designed configuration and that its role/indicator logic behaves for
 * a QuestGiver, plus that the canonical FARPGAttributeInitRow design contract
 * (the values persisted app-side in seed-characters.ts) holds.
 *
 * Pure config gate (no PIE/world needed) — the actor is constructed in the
 * transient package and only world-free getters are exercised. Runs headless:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests Project.Functional Tests.PoF.CharacterVael;Quit" -unattended -nopause -nullrhi -log
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSCharacterVaelTest,
	"Project.Functional Tests.PoF.CharacterVael.NPCConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSCharacterVaelTest::RunTest(const FString& /*Parameters*/)
{
	// --- NPC identity & role config (real production type, no .uasset needed) ---
	AARPGNPCActor* Vael = NewObject<AARPGNPCActor>(GetTransientPackage());
	if (!TestNotNull(TEXT("AARPGNPCActor instantiates"), Vael))
	{
		return false;
	}

	Vael->NPCID = FName(TEXT("CaptainVael"));
	Vael->DisplayName = FText::FromString(TEXT("Captain Vael"));
	Vael->NPCRole = EARPGNPCRole::QuestGiver;
	Vael->bFacePlayerInDialogue = true;

	// 1. Property roundtrip.
	TestTrue(TEXT("NPCID roundtrips"), Vael->NPCID == FName(TEXT("CaptainVael")));
	TestEqual(TEXT("DisplayName roundtrips"),
		Vael->DisplayName.ToString(), FString(TEXT("Captain Vael")));
	TestEqual(TEXT("NPCRole is QuestGiver"),
		static_cast<int32>(Vael->NPCRole),
		static_cast<int32>(EARPGNPCRole::QuestGiver));
	TestTrue(TEXT("Faces player in dialogue"), Vael->bFacePlayerInDialogue);

	// 2. Role/indicator logic behaves for a QuestGiver (exercises real code).
	TestEqual(TEXT("QuestGiver role display text"),
		Vael->GetRoleDisplayText().ToString(), FString(TEXT("Quest")));
	TestEqual(TEXT("QuestGiver indicator symbol is '!'"),
		Vael->GetIndicatorSymbol(), FString(TEXT("!")));
	// No GameInstance/world here, so the dynamic symbol falls back to the static one.
	TestEqual(TEXT("Dynamic indicator falls back to '!' without quest state"),
		Vael->GetDynamicIndicatorSymbol(), FString(TEXT("!")));
	const FLinearColor RoleColor = Vael->GetRoleColor();
	TestTrue(TEXT("QuestGiver role color is the gold accent"),
		RoleColor.Equals(FLinearColor(1.f, 0.85f, 0.f), 0.001f));

	// 3. Canonical attribute contract (must match seed-characters.ts design data).
	FARPGAttributeInitRow Stats;
	Stats.MaxHealth = 220.f;
	Stats.Health = 220.f;
	Stats.MaxMana = 60.f;
	Stats.Mana = 60.f;
	Stats.Strength = 16.f;
	Stats.Dexterity = 12.f;
	Stats.Intelligence = 11.f;
	Stats.Armor = 30.f;
	Stats.AttackPower = 24.f;
	Stats.CriticalChance = 0.08f;
	Stats.CriticalDamage = 1.6f;
	Stats.CharacterLevel = 5.f;

	TestEqual(TEXT("Canonical MaxHealth = 220"), Stats.MaxHealth, 220.f);
	TestEqual(TEXT("Spawns at full health"), Stats.Health, Stats.MaxHealth);
	TestEqual(TEXT("Spawns at full mana"), Stats.Mana, Stats.MaxMana);
	TestEqual(TEXT("Canonical CharacterLevel = 5"), Stats.CharacterLevel, 5.f);
	TestTrue(TEXT("Plate-armored captain has armor"), Stats.Armor > 0.f);
	TestTrue(TEXT("Crit chance within [0,1]"),
		Stats.CriticalChance >= 0.f && Stats.CriticalChance <= 1.f);
	TestTrue(TEXT("Crit damage is a multiplier >= 1"), Stats.CriticalDamage >= 1.f);

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

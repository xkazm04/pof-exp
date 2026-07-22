#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ARPGEnemyCharacter.h"
#include "AbilitySystem/ARPGGameplayTags.h"

/**
 * Bestiary gate — the arena-duel Sith archetypes (bestiary-sith-lord /
 * bestiary-sith-acolyte / bestiary-dark-marauder). Asserts the code-as-data
 * GetArchetypeDefaults values mirror the catalog Stat Blocks (moveSpeed
 * 600/570/630 cm/s) and the SOR XP rarity classes (rare x5 / magic x2),
 * following the BruteArchetypeConfig pattern. Headless:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests Project.Functional Tests.PoF.Bestiary.DuelArchetypesConfig;Quit" -unattended -nopause -nullrhi -log
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSBestiaryDuelArchetypesTest,
	"Project.Functional Tests.PoF.Bestiary.DuelArchetypesConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSBestiaryDuelArchetypesTest::RunTest(const FString& /*Parameters*/)
{
	const FEnemyArchetypeDefaults Lord =
		AARPGEnemyCharacter::GetArchetypeDefaults(EEnemyArchetype::SithLord);
	TestEqual(TEXT("SithLord moveSpeed is the Stat Block's 600 cm/s"), Lord.MoveSpeed, 600.f);
	TestEqual(TEXT("SithLord loot rarity bonus is the RARE class x5"), Lord.LootRarityBonusMultiplier, 5.f);
	TestTrue(TEXT("SithLord primary ability is the melee light attack"),
		Lord.PrimaryAbilityTag == ARPGGameplayTags::Ability_Melee_LightAttack.GetTag());

	const FEnemyArchetypeDefaults Acolyte =
		AARPGEnemyCharacter::GetArchetypeDefaults(EEnemyArchetype::SithAcolyte);
	TestEqual(TEXT("SithAcolyte moveSpeed is the Stat Block's 570 cm/s"), Acolyte.MoveSpeed, 570.f);
	TestEqual(TEXT("SithAcolyte loot rarity bonus is the MAGIC class x2"), Acolyte.LootRarityBonusMultiplier, 2.f);

	const FEnemyArchetypeDefaults Marauder =
		AARPGEnemyCharacter::GetArchetypeDefaults(EEnemyArchetype::DarkMarauder);
	TestEqual(TEXT("DarkMarauder moveSpeed is the Stat Block's 630 cm/s (reckless closer)"), Marauder.MoveSpeed, 630.f);
	TestTrue(TEXT("DarkMarauder attacks faster than the SithAcolyte"),
		Marauder.AttackCooldown < Acolyte.AttackCooldown);

	// The pre-duel archetypes must not disturb the existing roster's tuning.
	const FEnemyArchetypeDefaults Brute =
		AARPGEnemyCharacter::GetArchetypeDefaults(EEnemyArchetype::Brute);
	TestEqual(TEXT("Brute AttackRange stays 250 (regression)"), Brute.AttackRange, 250.f);
	TestEqual(TEXT("Brute keeps no MoveSpeed override (class default)"), Brute.MoveSpeed, 0.f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

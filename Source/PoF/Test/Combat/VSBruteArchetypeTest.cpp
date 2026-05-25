#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ARPGEnemyCharacter.h"
#include "AbilitySystem/ARPGGameplayTags.h"

/**
 * Combat test gate for the Stone Brute bestiary archetype (Catalog Pipeline,
 * Core/Existing Bestiary row — target asset "Brute"). Asserts the canonical
 * tuning that AARPGEnemyCharacter applies at possession, read from the
 * single-source-of-truth GetArchetypeDefaults() so the gate matches runtime
 * exactly without spinning up a PIE world:
 *   - AttackRange 250, AttackCooldown 4.0s,
 *   - charge vulnerability window 2.0s, charge speed x3,
 *   - primary ability Ability.Melee.HeavyAttack,
 *   - 2 loot rolls @ x2 rarity bonus, base XP 25,
 *   - melee bruiser (no preferred distance, never retreats).
 *
 * These mirror the app-side ENEMY_ARCHETYPES['brute'] design data and the
 * DEFAULT_ENEMY_LOOT_BINDINGS 'Brute' -> LT_Brute binding.
 *
 * Pure config gate (no PIE/world) — runs headless via:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests Project.Functional Tests.PoF.Bestiary;Quit" -unattended -nopause -nullrhi -log -abslog=BruteGate.log
 * (judge by the -abslog contents: the headless editor exits non-zero on a benign shutdown crash).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSBruteArchetypeTest,
	"Project.Functional Tests.PoF.Bestiary.BruteArchetypeConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSBruteArchetypeTest::RunTest(const FString& /*Parameters*/)
{
	const FEnemyArchetypeDefaults D =
		AARPGEnemyCharacter::GetArchetypeDefaults(EEnemyArchetype::Brute);

	// Combat reach & cadence
	TestEqual(TEXT("Brute AttackRange is 250"), D.AttackRange, 250.f);
	TestEqual(TEXT("Brute AttackCooldown is 4.0s"), D.AttackCooldown, 4.0f);

	// Signature charge mechanic (bait-the-charge, punish-the-recovery)
	TestEqual(TEXT("Brute charge vulnerability window is 2.0s"), D.ChargeVulnerabilityDuration, 2.0f);
	TestEqual(TEXT("Brute charge speed multiplier is x3"), D.ChargeSpeedMultiplier, 3.0f);

	// Primary attack identity
	TestTrue(TEXT("Brute primary ability is Ability.Melee.HeavyAttack"),
		D.PrimaryAbilityTag == ARPGGameplayTags::Ability_Melee_HeavyAttack);

	// Loot generosity (elite) + progression reward
	TestEqual(TEXT("Brute rolls loot twice"), D.LootNumRolls, 2);
	TestEqual(TEXT("Brute loot rarity bonus is x2"), D.LootRarityBonusMultiplier, 2.0f);
	TestEqual(TEXT("Brute base XP reward is 25"), D.BaseXPReward, 25.f);

	// Role contract: a melee bruiser, not a ranged kiter
	TestEqual(TEXT("Brute has no preferred combat distance (does not kite)"), D.PreferredCombatDistance, 0.f);
	TestEqual(TEXT("Brute does not retreat"), D.RetreatDistance, 0.f);

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

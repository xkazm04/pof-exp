#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Economy/ARPGCraftingRules.h"

/**
 * Crafting catalog L3 gate (crafting-recipes, runtimeDeferred('VSCraftingTest')).
 * Asserts the production craft-gate rules for the Alchemist's Bench health potion:
 * bench heal 120 (50 base * 2.4), and the gate fails on insufficient gold, low skill,
 * wrong station, or missing reagents (Test Gate checks #3-#7). Pure logic — headless.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSCraftingTest,
	"Project.Functional Tests.PoF.Crafting.VSCraftingTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSCraftingTest::RunTest(const FString& /*Parameters*/)
{
	// Output potency: bench upgrades 50 HP base to 120 HP (2.4x).
	TestEqual(TEXT("bench heal = 120 (50 * 2.4)"), UARPGCraftingRules::CraftedHealAmount(), 120);
	TestEqual(TEXT("gold cost = 20"), UARPGCraftingRules::GoldCost, 20);
	TestEqual(TEXT("required skill = 1"), UARPGCraftingRules::RequiredSkill, 1);

	// Happy path.
	TestTrue(TEXT("craft succeeds with gold+skill+station+reagents"),
		UARPGCraftingRules::CanCraft(20, 1, true, true));

	// Failure modes (each independently blocks the craft).
	TestFalse(TEXT("fails when gold < 20"), UARPGCraftingRules::CanCraft(19, 1, true, true));
	TestFalse(TEXT("fails when CraftingSkill < 1"), UARPGCraftingRules::CanCraft(20, 0, true, true));
	TestFalse(TEXT("fails at wrong (non-alchemist) station"), UARPGCraftingRules::CanCraft(20, 1, false, true));
	TestFalse(TEXT("fails when reagents missing"), UARPGCraftingRules::CanCraft(20, 1, true, false));

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

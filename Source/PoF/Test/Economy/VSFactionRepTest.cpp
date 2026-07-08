#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Economy/ARPGFactionRules.h"

/**
 * Faction catalog L3 gate (catalog pipeline: factions, "Test Gate" →
 * runtimeDeferred('VSFactionRepTest')). Asserts the real production faction rules
 * (UARPGFactionRules) honor the app-side Standing & Rep Tiers contract exactly:
 * tier thresholds, vendor discounts, and top-tier decay floored at Honored.
 *
 * Pure logic gate — no world/PIE needed; runs headless:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests VSFactionRepTest;Quit"
 *     -unattended -nopause -nullrhi -log
 * The pretty name embeds "VSFactionRepTest" so `Automation RunTests VSFactionRepTest`
 * (the drain's requested name) substring-matches it.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSFactionRepTest,
	"Project.Functional Tests.PoF.Factions.VSFactionRepTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSFactionRepTest::RunTest(const FString& /*Parameters*/)
{
	using T = EARPGFactionTier;

	// 1. Tier thresholds — boundaries + interiors (inclusive lower bounds).
	TestEqual(TEXT("-10000 → Hated"), UARPGFactionRules::GetTierForReputation(-10000), T::Hated);
	TestEqual(TEXT("-3000 → Unfriendly"), UARPGFactionRules::GetTierForReputation(-3000), T::Unfriendly);
	TestEqual(TEXT("0 → Neutral"), UARPGFactionRules::GetTierForReputation(0), T::Neutral);
	TestEqual(TEXT("+3000 from Neutral → Friendly"), UARPGFactionRules::GetTierForReputation(3000), T::Friendly);
	TestEqual(TEXT("5999 stays Friendly"), UARPGFactionRules::GetTierForReputation(5999), T::Friendly);
	TestEqual(TEXT("6000 → Honored"), UARPGFactionRules::GetTierForReputation(6000), T::Honored);
	TestEqual(TEXT("8999 stays Honored"), UARPGFactionRules::GetTierForReputation(8999), T::Honored);
	TestEqual(TEXT("9000 → Revered"), UARPGFactionRules::GetTierForReputation(9000), T::Revered);
	TestEqual(TEXT("12000 → Exalted"), UARPGFactionRules::GetTierForReputation(12000), T::Exalted);
	// -1000 from Friendly (3000) → net 2000 → Neutral (Test Gate check #5).
	TestEqual(TEXT("net 2000 → Neutral"), UARPGFactionRules::GetTierForReputation(2000), T::Neutral);

	// 2. Vendor discounts (Test Gate checks #3, #4).
	TestEqual(TEXT("Exalted discount = 20%"), UARPGFactionRules::GetDiscountPercent(T::Exalted), 20);
	TestEqual(TEXT("Friendly discount = 5%"), UARPGFactionRules::GetDiscountPercent(T::Friendly), 5);
	TestEqual(TEXT("Honored discount = 10%"), UARPGFactionRules::GetDiscountPercent(T::Honored), 10);
	TestEqual(TEXT("Revered discount = 15%"), UARPGFactionRules::GetDiscountPercent(T::Revered), 15);
	TestEqual(TEXT("Unfriendly = -5% surcharge"), UARPGFactionRules::GetDiscountPercent(T::Unfriendly), -5);
	TestEqual(TEXT("Neutral = 0%"), UARPGFactionRules::GetDiscountPercent(T::Neutral), 0);

	// 3. Passive decay: top tiers only, floored at Honored (Test Gate check #7).
	TestEqual(TEXT("Revered decays 10/day"), UARPGFactionRules::GetDecayPerDay(T::Revered), 10);
	TestEqual(TEXT("Exalted decays 10/day"), UARPGFactionRules::GetDecayPerDay(T::Exalted), 10);
	TestEqual(TEXT("Honored does not decay"), UARPGFactionRules::GetDecayPerDay(T::Honored), 0);
	TestEqual(TEXT("Friendly does not decay"), UARPGFactionRules::GetDecayPerDay(T::Friendly), 0);
	TestTrue(TEXT("decay floor is Honored's minPoints (6000)"), UARPGFactionRules::HonoredFloorPoints == 6000);

	// 4. Hated closes all services (Test Gate check #6).
	TestFalse(TEXT("Hated: services closed"), UARPGFactionRules::AreServicesOpen(T::Hated));
	TestTrue(TEXT("Neutral: services open"), UARPGFactionRules::AreServicesOpen(T::Neutral));

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

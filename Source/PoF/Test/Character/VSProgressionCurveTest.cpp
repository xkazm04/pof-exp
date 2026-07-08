#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/ARPGProgressionRules.h"

/**
 * Progression catalog L3 gate (progression-curves, runtimeDeferred('VSProgressionCurveTest')).
 * Asserts the production XP-curve rules honor the app contract: geometric
 * xpToNext(L)=100*1.08^L at L1/L10/L50/L90, soft cap clamps growth at L90.
 * Pure math — headless, no PIE.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSProgressionCurveTest,
	"Project.Functional Tests.PoF.Progression.VSProgressionCurveTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSProgressionCurveTest::RunTest(const FString& /*Parameters*/)
{
	// Sample values from the pipeline contract (progression-curves.ts).
	TestEqual(TEXT("L1 = 108"), UARPGProgressionRules::XpToNextLevel(1), 108);
	TestEqual(TEXT("L10 = 216"), UARPGProgressionRules::XpToNextLevel(10), 216);
	TestEqual(TEXT("L50 = 4690"), UARPGProgressionRules::XpToNextLevel(50), 4690);

	// L90 ≈ 101892 (rounding of the growth base) — assert within 1%.
	const int32 L90 = UARPGProgressionRules::XpToNextLevel(90);
	TestTrue(TEXT("L90 within 1% of 101892"),
		FMath::Abs(L90 - 101892) <= 1019);

	// Soft cap: xpToNext(91..100) clamps to xpToNext(90).
	TestEqual(TEXT("softCap: xpToNext(91) == xpToNext(90)"),
		UARPGProgressionRules::XpToNextLevel(91), L90);
	TestEqual(TEXT("hardCap: xpToNext(100) == xpToNext(90)"),
		UARPGProgressionRules::XpToNextLevel(100), L90);

	// Monotonic increasing below the cap.
	TestTrue(TEXT("curve is monotonically increasing"),
		UARPGProgressionRules::XpToNextLevel(2) > UARPGProgressionRules::XpToNextLevel(1)
		&& UARPGProgressionRules::XpToNextLevel(50) > UARPGProgressionRules::XpToNextLevel(10));

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

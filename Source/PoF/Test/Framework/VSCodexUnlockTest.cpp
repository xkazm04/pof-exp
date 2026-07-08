#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Framework/ARPGCodexRules.h"

/**
 * Codex L3 gate (codex, runtimeDeferred('VSCodexUnlockTest')). Asserts the codex
 * entry unlock logic: hidden before either trigger, visible after quest stage 1
 * (primary) OR zone entry (fallback). Pure logic — headless.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSCodexUnlockTest,
	"Project.Functional Tests.PoF.Codex.VSCodexUnlockTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSCodexUnlockTest::RunTest(const FString& /*Parameters*/)
{
	// Hidden before either trigger (quest stage 0, zone not entered).
	TestFalse(TEXT("hidden before quest stage 1 and before zone entry"),
		UARPGCodexRules::ShouldUnlockEntry(0, false));
	// Primary path: quest reaches stage 1.
	TestTrue(TEXT("visible after quest-ember-pact stage 1 (primary)"),
		UARPGCodexRules::ShouldUnlockEntry(1, false));
	// Fallback path: Ashen Forest zone entry.
	TestTrue(TEXT("visible after Ashen Forest zone entry (fallback)"),
		UARPGCodexRules::ShouldUnlockEntry(0, true));
	// Either alone suffices; both is still unlocked.
	TestTrue(TEXT("both triggers → unlocked"),
		UARPGCodexRules::ShouldUnlockEntry(2, true));

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

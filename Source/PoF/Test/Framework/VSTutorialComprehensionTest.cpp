#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Framework/ARPGTutorialRules.h"

/**
 * Tutorial-beats L3 gate (tutorial-beats, runtimeDeferred('VSTutorialComprehensionTest')).
 * Asserts the beat gating: fires while the Introduced tag is absent, never once set
 * (idempotent), and offensive inputs are locked during the sandbox. Pure logic — headless.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSTutorialComprehensionTest,
	"Project.Functional Tests.PoF.Tutorial.VSTutorialComprehensionTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSTutorialComprehensionTest::RunTest(const FString& /*Parameters*/)
{
	// Beat fires on zone entry only while the Introduced tag is absent.
	TestTrue(TEXT("beat triggers when Tutorial.Dodge.Introduced is absent"),
		UARPGTutorialRules::ShouldTriggerBeat(false));
	TestFalse(TEXT("beat does NOT trigger once Introduced is set (idempotent)"),
		UARPGTutorialRules::ShouldTriggerBeat(true));

	// Offensive inputs locked while the sandbox beat is active.
	TestTrue(TEXT("IA_Attack/IA_Skill locked while sandbox active"),
		UARPGTutorialRules::AreOffensiveInputsLocked(true));
	TestFalse(TEXT("inputs unlocked when sandbox inactive"),
		UARPGTutorialRules::AreOffensiveInputsLocked(false));

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Framework/ARPGCutsceneRules.h"

/**
 * Cutscenes L3 gate (cutscenes, runtimeDeferred('VSCutsceneTimingTest')). Asserts the
 * prologue timing contract: beat markers at 38/58/72/82 s (ordered, inside a 90 s
 * sequence) and skippable only after the 3 s grace. Pure logic — headless.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSCutsceneTimingTest,
	"Project.Functional Tests.PoF.Cutscenes.VSCutsceneTimingTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSCutsceneTimingTest::RunTest(const FString& /*Parameters*/)
{
	// Declared beat timecodes (cutscenes.ts).
	TestEqual(TEXT("VaelIntroEnter @ 38s"), UARPGCutsceneRules::TcVaelIntroEnter, 38.f);
	TestEqual(TEXT("VaelEyeContact @ 58s"), UARPGCutsceneRules::TcVaelEyeContact, 58.f);
	TestEqual(TEXT("EmberCrescendo @ 72s"), UARPGCutsceneRules::TcEmberCrescendo, 72.f);
	TestEqual(TEXT("End @ 82s"), UARPGCutsceneRules::TcEnd, 82.f);
	TestEqual(TEXT("total duration 90s"), UARPGCutsceneRules::TotalDurationSeconds, 90.f);

	// Beats ordered and inside the sequence.
	TestTrue(TEXT("beat markers strictly ordered within [0,90]"),
		UARPGCutsceneRules::BeatsAreValid());

	// Skippable only after the 3s grace window.
	TestFalse(TEXT("not skippable at t=2s (inside grace)"), UARPGCutsceneRules::IsSkippable(2.f));
	TestTrue(TEXT("skippable at t=3s (grace elapsed)"), UARPGCutsceneRules::IsSkippable(3.f));
	TestTrue(TEXT("skippable at t=45s"), UARPGCutsceneRules::IsSkippable(45.f));

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

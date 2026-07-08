#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/ARPGAmbientRules.h"

/**
 * Ambient L3 gate (ambient, runtimeDeferred('VSAmbientTest')). Asserts the deterministic
 * soundscape config: bed gain −6 dB, 3 detail emitters, ember one-shot within a ≤20 s
 * interval. Actual PIE playback/spatialization is the runtime concern.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSAmbientTest,
	"Project.Functional Tests.PoF.Ambient.VSAmbientTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSAmbientTest::RunTest(const FString& /*Parameters*/)
{
	TestEqual(TEXT("3 detail-loop emitters"), UARPGAmbientRules::DetailEmitterCount, 3);

	// Bed gain calibrated to −6 dB.
	TestTrue(TEXT("bed gain −6 dB is calibrated"), UARPGAmbientRules::IsBedGainCalibrated(-6.f));
	TestFalse(TEXT("−3 dB is not the calibrated bed gain"), UARPGAmbientRules::IsBedGainCalibrated(-3.f));

	// Ember one-shot fires within the first 20 s interval.
	TestTrue(TEXT("ember fires within first interval (18s)"),
		UARPGAmbientRules::EmberFiresWithinFirstInterval(18.f));
	TestFalse(TEXT("ember not within interval at 21s"),
		UARPGAmbientRules::EmberFiresWithinFirstInterval(21.f));

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

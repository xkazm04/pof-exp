#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/ARPGMusicRules.h"

/**
 * Music L3 gate (music, runtimeDeferred('VSMusicTransitionTest')). Asserts the
 * deterministic MusicEvent→layer mapping and the boss-swell HP trigger. Beat-sync/
 * crossfade timing is the runtime concern; this encodes the transition decisions.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSMusicTransitionTest,
	"Project.Functional Tests.PoF.Music.VSMusicTransitionTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSMusicTransitionTest::RunTest(const FString& /*Parameters*/)
{
	using L = EARPGMusicLayer;

	TestEqual(TEXT("AmbientStart → AmbientTension"),
		UARPGMusicRules::LayerForEvent(FName(TEXT("MusicEvent.AmbientStart"))), L::AmbientTension);
	TestEqual(TEXT("CombatStart → CombatLow"),
		UARPGMusicRules::LayerForEvent(FName(TEXT("MusicEvent.CombatStart"))), L::CombatLow);
	TestEqual(TEXT("EliteSpawned → CombatHigh"),
		UARPGMusicRules::LayerForEvent(FName(TEXT("MusicEvent.EliteSpawned"))), L::CombatHigh);
	TestEqual(TEXT("BossSwell → BossSwell"),
		UARPGMusicRules::LayerForEvent(FName(TEXT("MusicEvent.BossSwell"))), L::BossSwell);
	TestEqual(TEXT("unknown event → Silence"),
		UARPGMusicRules::LayerForEvent(FName(TEXT("MusicEvent.Nonsense"))), L::Silence);

	// Boss-swell trigger at ≤30% elite HP.
	TestTrue(TEXT("boss-swell at 30% HP"), UARPGMusicRules::ShouldBossSwell(0.30f));
	TestTrue(TEXT("boss-swell at 15% HP"), UARPGMusicRules::ShouldBossSwell(0.15f));
	TestFalse(TEXT("no boss-swell at 31% HP"), UARPGMusicRules::ShouldBossSwell(0.31f));

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

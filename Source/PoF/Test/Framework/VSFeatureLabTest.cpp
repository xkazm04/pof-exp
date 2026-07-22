#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "World/PoFFeatureLabSubsystem.h"

/**
 * FeatureLab gate — the clean default map's runtime roster. Config gate on the
 * code-as-data population (no PIE): map-name gating behaves (PIE prefixes
 * tolerated, other maps untouched) and the roster carries the Duel Challenge
 * speaker. Grows with every feature added to the lab. Headless:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests Project.Functional Tests.PoF.FeatureLab.RosterConfig;Quit" -unattended -nopause -nullrhi -log
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSFeatureLabTest,
	"Project.Functional Tests.PoF.FeatureLab.RosterConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSFeatureLabTest::RunTest(const FString& /*Parameters*/)
{
	// Map gating — the subsystem must populate ONLY the FeatureLab.
	TestTrue(TEXT("Standalone map name populates"),
		UPoFFeatureLabSubsystem::ShouldPopulate(TEXT("FeatureLab")));
	TestTrue(TEXT("PIE-prefixed map name populates"),
		UPoFFeatureLabSubsystem::ShouldPopulate(TEXT("UEDPIE_0_FeatureLab")));
	TestFalse(TEXT("VerticalSlice is untouched"),
		UPoFFeatureLabSubsystem::ShouldPopulate(TEXT("VerticalSlice")));
	TestFalse(TEXT("OpenWorld is untouched"),
		UPoFFeatureLabSubsystem::ShouldPopulate(TEXT("OpenWorld")));

	// Roster contract — the Duel Challenge speaker is present and well-formed.
	const TArray<FFeatureLabEntry> Roster = UPoFFeatureLabSubsystem::GetRoster();
	TestTrue(TEXT("Roster is non-empty"), Roster.Num() > 0);

	const FFeatureLabEntry* Malgrave = Roster.FindByPredicate(
		[](const FFeatureLabEntry& E) { return E.NPCID == FName(TEXT("Malgrave")); });
	if (TestNotNull(TEXT("Malgrave (Duel Challenge speaker) is in the roster"), Malgrave))
	{
		TestEqual(TEXT("Malgrave is an ARPGNPCActor"),
			Malgrave->ClassPath, FString(TEXT("/Script/PoF.ARPGNPCActor")));
		TestTrue(TEXT("Malgrave spawns within interaction reach of the start (< 800 uu)"),
			Malgrave->Offset.Size() < 800.f);
	}

	// Every entry must name a /Script class (data errors fail here, not at spawn).
	for (const FFeatureLabEntry& E : Roster)
	{
		TestTrue(FString::Printf(TEXT("Entry '%s' has a /Script class path"), *E.Label),
			E.ClassPath.StartsWith(TEXT("/Script/")));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

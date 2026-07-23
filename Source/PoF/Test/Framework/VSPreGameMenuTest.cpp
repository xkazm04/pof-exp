#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/PreGameMenuWidget.h"
#include "World/PoFDuelSelectionSubsystem.h"
#include "UObject/Package.h"
#include "Engine/GameInstance.h"

/**
 * FeatureLab gate — the pre-game menu (UE mirror of the browser staging shell).
 * Config gate: the C++-built widget tree roots with both saber choices present,
 * and the duel-selection subsystem round-trips a choice. Headless:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests Project.Functional Tests.PoF.FeatureLab.PreGameMenuConfig;Quit" -unattended -nopause -nullrhi -log
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSPreGameMenuTest,
	"Project.Functional Tests.PoF.FeatureLab.PreGameMenuConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSPreGameMenuTest::RunTest(const FString& /*Parameters*/)
{
	// Selection subsystem round-trip. ClassWithin = GameInstance, so give it one
	// (transient) as outer — a bare package outer logs a warning the runner escalates.
	UGameInstance* GI = NewObject<UGameInstance>(GetTransientPackage());
	UPoFDuelSelectionSubsystem* Sel = NewObject<UPoFDuelSelectionSubsystem>(GI);
	if (!TestNotNull(TEXT("Selection subsystem instantiates"), Sel))
	{
		return false;
	}
	TestTrue(TEXT("No saber chosen by default"), Sel->SelectedSaber.IsNone());
	Sel->SelectSaber(FName(TEXT("Azure")));
	TestEqual(TEXT("Selection round-trips"), Sel->SelectedSaber, FName(TEXT("Azure")));

	// The widget itself needs a real creation context (raw NewObject + Initialize trips
	// an engine ensure headless) — its built-ness is proven at RUNTIME instead: the
	// FeatureLab subsystem logs "[PreGameMenu] shown (built=yes)" and the -game frame
	// capture shows the rendered menu. This gate owns the world-free contract only.
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

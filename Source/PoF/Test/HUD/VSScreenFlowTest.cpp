#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "UI/ARPGScreenFlowRules.h"

/**
 * Screen-flow L3 gate (screen-flow, runtimeDeferred('VSScreenFlowTest')). Asserts the
 * deterministic z-ordering contract: overlays above HUD (z3 > z1), Pause above overlays
 * (z4 > z3), HUD dimmed to 50% behind overlays. Pure logic — headless.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSScreenFlowTest,
	"Project.Functional Tests.PoF.ScreenFlow.VSScreenFlowTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSScreenFlowTest::RunTest(const FString& /*Parameters*/)
{
	TestEqual(TEXT("HUD z-depth = 1"), UARPGScreenFlowRules::HudZDepth, 1);
	TestEqual(TEXT("overlay z-depth = 3"), UARPGScreenFlowRules::OverlayZDepth, 3);
	TestEqual(TEXT("pause z-depth = 4"), UARPGScreenFlowRules::PauseZDepth, 4);

	TestTrue(TEXT("overlays render above the HUD"), UARPGScreenFlowRules::OverlaysRenderAboveHud());
	TestTrue(TEXT("pause renders above overlays"), UARPGScreenFlowRules::PauseRendersAboveOverlays());

	TestEqual(TEXT("HUD dims to 50% behind an overlay"),
		UARPGScreenFlowRules::HudOpacityBehindOverlay, 0.5f);

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

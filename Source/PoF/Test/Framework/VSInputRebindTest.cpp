#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Input/ARPGInputRebindRules.h"

/**
 * Input-schemes L3 gate (input-schemes, runtimeDeferred('VSInputRebindTest')). Asserts
 * the deterministic rebind rules: a key already bound to another action conflicts (no
 * silent overwrite), and deadzone is clamped to a valid range. Pure logic — headless.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSInputRebindTest,
	"Project.Functional Tests.PoF.InputSchemes.VSInputRebindTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSInputRebindTest::RunTest(const FString& /*Parameters*/)
{
	TArray<FName> Bound = { FName(TEXT("Gamepad_FaceButton_Bottom")), FName(TEXT("Gamepad_FaceButton_Right")) };

	// Conflict rejected: rebinding to an already-used key.
	TestTrue(TEXT("conflict when key already bound to another action"),
		UARPGInputRebindRules::WouldConflict(Bound, FName(TEXT("Gamepad_FaceButton_Bottom"))));
	// Free key: no conflict.
	TestFalse(TEXT("no conflict for an unused key"),
		UARPGInputRebindRules::WouldConflict(Bound, FName(TEXT("Gamepad_FaceButton_Left"))));

	// Deadzone clamps immediately into [0, 0.95].
	TestEqual(TEXT("deadzone clamps high to 0.95"), UARPGInputRebindRules::ClampDeadzone(1.5f), 0.95f);
	TestEqual(TEXT("deadzone clamps negative to 0"), UARPGInputRebindRules::ClampDeadzone(-0.2f), 0.f);
	TestEqual(TEXT("deadzone passes a valid value"), UARPGInputRebindRules::ClampDeadzone(0.3f), 0.3f);

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

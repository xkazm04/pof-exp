#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Framework/ARPGAchievementRules.h"

/**
 * Achievement catalog L3 gate (achievements, runtimeDeferred('VSAchievementTest')).
 * Asserts the production First Blood rules: no unlock at killCount 0, unlock at
 * threshold 1, idempotent (a second kill after unlock does NOT re-grant), and the
 * reward is 100 gold + 1 item-7. Pure logic — headless (Test Gate checks #1-#3, #5-#6).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSAchievementTest,
	"Project.Functional Tests.PoF.Achievements.VSAchievementTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSAchievementTest::RunTest(const FString& /*Parameters*/)
{
	const int32 Threshold = UARPGAchievementRules::FirstBloodThreshold;
	TestEqual(TEXT("First Blood threshold = 1"), Threshold, 1);

	// #1 does NOT unlock before any kill.
	TestFalse(TEXT("killCount 0 → no unlock"),
		UARPGAchievementRules::ShouldUnlock(0, Threshold, false));
	// #2 killing one enemy fires the unlock.
	TestTrue(TEXT("killCount 1 → unlock"),
		UARPGAchievementRules::ShouldUnlock(1, Threshold, false));
	// #3 idempotent — a second kill after unlock does not re-grant.
	TestFalse(TEXT("already unlocked → second kill does not re-grant"),
		UARPGAchievementRules::ShouldUnlock(2, Threshold, true));

	// #5/#6 reward payload.
	TestEqual(TEXT("reward = 100 gold"), UARPGAchievementRules::RewardGold, 100);
	TestEqual(TEXT("reward item quantity = 1"), UARPGAchievementRules::RewardItemQuantity, 1);
	TestTrue(TEXT("reward item is item-7"),
		UARPGAchievementRules::RewardItemId() == FName(TEXT("item-7")));

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

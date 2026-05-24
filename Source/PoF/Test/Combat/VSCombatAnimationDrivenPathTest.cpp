#include "Test/Combat/VSCombatAnimationDrivenPathTest.h"

AVSCombatAnimationDrivenPathTest::AVSCombatAnimationDrivenPathTest()
{
	PrimaryActorTick.bCanEverTick = false;
	// Disabled: the automation runner skips tests with bIsEnabled=false.
	bIsEnabled = false;
	TimeLimit = 10.f;
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AVSCombatAnimationDrivenPathTest::StartTest()
{
	Super::StartTest();
	// Belt-and-suspenders in case the test is placed/run despite bIsEnabled=false.
	FinishTest(EFunctionalTestResult::Default,
		TEXT("disabled: needs a real AM_MeleeCombo montage with AnimNotifyState_HitDetection (folder-03 tests UE §2)"));
}

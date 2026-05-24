#include "Test/Environment/VSArenaSetupTest.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"

AVSArenaSetupTest::AVSArenaSetupTest()
{
	TimeLimit = 10.f;
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AVSArenaSetupTest::StartTest()
{
	Super::StartTest();

	int32 DirLights = 0, SkyLights = 0, PPVs = 0;
	for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It) ++DirLights;
	for (TActorIterator<ASkyLight> It(GetWorld()); It; ++It) ++SkyLights;
	for (TActorIterator<APostProcessVolume> It(GetWorld()); It; ++It) ++PPVs;

	AssertTrue(DirLights >= 1, FString::Printf(TEXT("#1 setup: DirectionalLight present (%d)"), DirLights));
	AssertTrue(SkyLights >= 1, FString::Printf(TEXT("#2 setup: SkyLight present (%d)"), SkyLights));
	AssertTrue(PPVs >= 1, FString::Printf(TEXT("#3 setup: PostProcessVolume present (%d)"), PPVs));

	FinishTest(EFunctionalTestResult::Default, TEXT("arena lighting/PP configured"));
}

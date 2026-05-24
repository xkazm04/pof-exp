#include "Test/Environment/VSFootstepWiringTest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundCue.h"

AVSFootstepWiringTest::AVSFootstepWiringTest()
{
	TimeLimit = 10.f;
	LogWarningHandling = EFunctionalTestLogHandling::OutputIgnored;
}

void AVSFootstepWiringTest::StartTest()
{
	Super::StartTest();

	const FString AudioFolderPath = TEXT("/Game/Audio/footstep-stone");
	const FString CuePath = AudioFolderPath + TEXT("/SC_footstep_stone.SC_footstep_stone");

	IAssetRegistry& Reg = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	TArray<FAssetData> Assets;
	Reg.GetAssetsByPath(FName(*AudioFolderPath), Assets, /*bRecursive*/ true);

	int32 NumWaves = 0;
	for (const FAssetData& A : Assets)
	{
		if (A.GetClass() && A.GetClass()->IsChildOf(USoundWave::StaticClass()))
		{
			++NumWaves;
		}
	}
	AssertTrue(NumWaves >= 1, FString::Printf(TEXT("#1 footstep-stone: at least 1 USoundWave imported (found %d)"), NumWaves));

	USoundCue* Cue = LoadObject<USoundCue>(nullptr, *CuePath);
	AssertTrue(Cue != nullptr, FString::Printf(TEXT("#2 footstep-stone: SC_footstep_stone cue exists at %s"), *CuePath));

	FinishTest(EFunctionalTestResult::Default, TEXT("footstep-stone import + cue present"));
}

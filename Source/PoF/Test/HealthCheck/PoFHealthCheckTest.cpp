#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/PackageName.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/UObjectGlobals.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Character/ARPGEnemyCharacter.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/Effects/GE_Damage.h"
#include "Test/ARPGFunctionalTestBase.h"

/**
 * Project invariant suite — runs first in any CI pass so a broken project
 * *structure* fails here, deterministically and in <30s, instead of surfacing
 * as confusing per-system functional-test failures downstream.
 *
 * Run with:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests Project.Functional Tests.PoF.HealthCheck;Quit" -unattended -nopause -nullrhi -log
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPoFHealthCheckTest,
	"Project.Functional Tests.PoF.HealthCheck",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoFHealthCheckTest::RunTest(const FString& /*Parameters*/)
{
	// --- 1. Core C++ classes link + are reflected ---
	TestNotNull(TEXT("AARPGPlayerCharacter class resolves"), AARPGPlayerCharacter::StaticClass());
	TestNotNull(TEXT("AARPGEnemyCharacter class resolves"), AARPGEnemyCharacter::StaticClass());
	TestNotNull(TEXT("UARPGAttributeSet class resolves"), UARPGAttributeSet::StaticClass());
	TestNotNull(TEXT("UGE_Damage class resolves"), UGE_Damage::StaticClass());
	TestNotNull(TEXT("AARPGFunctionalTestBase class resolves"), AARPGFunctionalTestBase::StaticClass());

	// --- 2. The slice level exists ---
	TestTrue(TEXT("slice map exists at /Game/Maps/VerticalSlice"),
		FPackageName::DoesPackageExist(TEXT("/Game/Maps/VerticalSlice")));

	// --- 3. DefaultGame.ini ProjectID is non-empty ---
	FString ProjectID;
	GConfig->GetString(TEXT("/Script/EngineSettings.GeneralProjectSettings"),
		TEXT("ProjectID"), ProjectID, GGameIni);
	TestFalse(TEXT("ProjectID is non-empty"), ProjectID.IsEmpty());

	// --- 4. GameDefaultMap is set and its package exists ---
	FString GameDefaultMap;
	GConfig->GetString(TEXT("/Script/EngineSettings.GameMapsSettings"),
		TEXT("GameDefaultMap"), GameDefaultMap, GEngineIni);
	TestFalse(TEXT("GameDefaultMap is set"), GameDefaultMap.IsEmpty());
	if (!GameDefaultMap.IsEmpty())
	{
		TestTrue(TEXT("GameDefaultMap package exists"),
			FPackageName::DoesPackageExist(GameDefaultMap));
	}

	// --- 5. The default GameMode loads and has its core classes wired ---
	FString GameModePath;
	GConfig->GetString(TEXT("/Script/EngineSettings.GameMapsSettings"),
		TEXT("GlobalDefaultGameMode"), GameModePath, GEngineIni);
	TestFalse(TEXT("GlobalDefaultGameMode is configured"), GameModePath.IsEmpty());

	if (!GameModePath.IsEmpty())
	{
		UClass* GameModeClass = StaticLoadClass(AGameModeBase::StaticClass(), nullptr, *GameModePath);
		TestNotNull(TEXT("GlobalDefaultGameMode class loads"), GameModeClass);

		if (GameModeClass)
		{
			const AGameModeBase* GameModeCDO = GameModeClass->GetDefaultObject<AGameModeBase>();
			if (TestNotNull(TEXT("GameMode CDO resolves"), GameModeCDO))
			{
				TestNotNull(TEXT("GameMode DefaultPawnClass is set"), GameModeCDO->DefaultPawnClass.Get());
				TestNotNull(TEXT("GameMode PlayerControllerClass is set"), GameModeCDO->PlayerControllerClass.Get());
				TestNotNull(TEXT("GameMode HUDClass is set"), GameModeCDO->HUDClass.Get());
			}
		}
	}

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

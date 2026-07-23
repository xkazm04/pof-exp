#pragma once

#include "CoreMinimal.h"
#include "Test/ARPGFunctionalTestBase.h"
#include "AVSHUDFunctionalTest.generated.h"

class UVSHUDWidget;

/**
 * Vertical-slice HUD functional test (place one instance in /Game/Maps/VerticalSlice).
 * Phases: HUDStructure -> HUDBinding -> DamageNumbers.
 * Run: Project.Functional Tests.Maps.VerticalSlice.VSHUDFunctionalTest
 */
UCLASS()
class POF_API AVSHUDFunctionalTest : public AARPGFunctionalTestBase
{
	GENERATED_BODY()

public:
	AVSHUDFunctionalTest();

protected:
	virtual void OnTestStarted() override;
	virtual EARPGPhaseResult RunPhase(int32 PhaseIndex, FName PhaseName, float DeltaSeconds) override;
	virtual void CleanUp() override;

private:
	UPROPERTY()
	UVSHUDWidget* Widget = nullptr;

	/** The enemy this test spawned as its own fixture (restored on CleanUp). */
	TWeakObjectPtr<AARPGEnemyCharacter> SpawnedFixture;
};

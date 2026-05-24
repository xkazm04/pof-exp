#pragma once

#include "CoreMinimal.h"
#include "Test/ARPGFunctionalTestBase.h"
#include "ARPGHUDWidgetTest.generated.h"

class UARPGHUDWidget;
class UAbilityBarWidget;

/**
 * Functional test for the reparented (pure-C++) real HUD — UARPGHUDWidget +
 * UAbilityBarWidget built via UARPGCodeWidgetBase::BuildTree(), no companion
 * Widget Blueprints. Place one instance in /Game/Maps/VerticalSlice.
 * Phases: HUDStructure -> Hotbar -> HitVignette -> HUDBinding.
 * Run: Project.Functional Tests.Maps.VerticalSlice.ARPGHUDWidgetTest
 */
UCLASS()
class POF_API AARPGHUDWidgetTest : public AARPGFunctionalTestBase
{
	GENERATED_BODY()

public:
	AARPGHUDWidgetTest();

protected:
	virtual void OnTestStarted() override;
	virtual EARPGPhaseResult RunPhase(int32 PhaseIndex, FName PhaseName, float DeltaSeconds) override;

private:
	UPROPERTY()
	UARPGHUDWidget* HUD = nullptr;

	UPROPERTY()
	UAbilityBarWidget* Bar = nullptr;
};

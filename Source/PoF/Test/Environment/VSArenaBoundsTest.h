#pragma once

#include "CoreMinimal.h"
#include "Test/ARPGFunctionalTestBase.h"
#include "VSArenaBoundsTest.generated.h"

/** Drives the player into a wall; asserts the wall blocks it inside the arena. */
UCLASS()
class POF_API AVSArenaBoundsTest : public AARPGFunctionalTestBase
{
	GENERATED_BODY()

public:
	AVSArenaBoundsTest();

protected:
	virtual void OnTestStarted() override;
	virtual EARPGPhaseResult RunPhase(int32 PhaseIndex, FName PhaseName, float DeltaSeconds) override;

private:
	FVector StartLoc = FVector::ZeroVector;
};

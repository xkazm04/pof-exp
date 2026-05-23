#pragma once

#include "CoreMinimal.h"
#include "Test/ARPGFunctionalTestBase.h"
#include "ProcGenWalkTest.generated.h"

/** Proves the generated dungeon is walkable: the player walks from the start room into a connected room. */
UCLASS()
class POF_API AProcGenWalkTest : public AARPGFunctionalTestBase
{
	GENERATED_BODY()

public:
	AProcGenWalkTest();

protected:
	virtual void OnTestStarted() override;
	virtual EARPGPhaseResult RunPhase(int32 PhaseIndex, FName PhaseName, float DeltaSeconds) override;

private:
	FVector TargetCenter = FVector::ZeroVector;
	FVector2D TargetHalfXY = FVector2D::ZeroVector;
	bool bResolved = false;
};

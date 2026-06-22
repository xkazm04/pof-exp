#pragma once

#include "CoreMinimal.h"
#include "Test/ARPGFunctionalTestBase.h"
#include "VSForcePushKnockbackTest.generated.h"

/**
 * Behavioral (PIE) gate for the Star Wars arena duel's Force Push.
 *
 * Places the Sith (the map's enemy) squarely in the player's forward cone within
 * Force Push range, fires the ability from hotbar slot 0 (the key '1' binding),
 * then asserts the enemy is physically launched away from its start — proving the
 * knockback actually fires in a live game world, not just that the config is right.
 */
UCLASS()
class AVSForcePushKnockbackTest : public AARPGFunctionalTestBase
{
	GENERATED_BODY()

public:
	AVSForcePushKnockbackTest();

protected:
	virtual void OnTestStarted() override;
	virtual EARPGPhaseResult RunPhase(int32 PhaseIndex, FName PhaseName, float DeltaSeconds) override;

private:
	FVector EnemyStartLoc = FVector::ZeroVector;
	bool bActivated = false;
};

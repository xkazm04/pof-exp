#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSEnemyAttackTest.generated.h"

/** Verifies the slice enemy is hostile: it activates its melee ability and the player takes GAS damage. */
UCLASS()
class POF_API AVSEnemyAttackTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AVSEnemyAttackTest();

	virtual void PrepareTest() override;
	virtual void StartTest() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	float PhaseTime = 0.f;
	float PlayerStartHealth = 0.f;
	TWeakObjectPtr<class AARPGEnemyCharacter> Enemy;
	TWeakObjectPtr<class AARPGPlayerCharacter> Player;
};

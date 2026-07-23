#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSCombatGrayBoxPathTest.generated.h"

class AARPGPlayerCharacter;
class AARPGEnemyCharacter;

/**
 * Gray-box melee damage path: with bUseAnimationDrivenDamage=false (the default),
 * activating GA_MeleeAttack with an enemy in front must damage that enemy WITHOUT
 * the test sending Event.MeleeHit. Guards the in-game hit path the empty montage
 * cannot exercise.
 */
UCLASS()
class POF_API AVSCombatGrayBoxPathTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AVSCombatGrayBoxPathTest();

	virtual void PrepareTest() override;
	virtual void StartTest() override;
	virtual void CleanUp() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	TWeakObjectPtr<AARPGPlayerCharacter> Player;
	TWeakObjectPtr<AARPGEnemyCharacter>  Enemy;
	/** Enemy this test spawned itself (destroyed in CleanUp). */
	TWeakObjectPtr<AARPGEnemyCharacter> SpawnedFixture;

	int32 Phase = 0;
	float PhaseTime = 0.f;
	float EnemyStartHealth = 0.f;
};

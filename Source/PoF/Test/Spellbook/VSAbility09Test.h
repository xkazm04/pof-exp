#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSAbility09Test.generated.h"

/**
 * Folder-09 generation proof gate.
 *
 * Grants the gray-box UGA_VS09Smite to the player at runtime (no binary config),
 * places the player next to the slice enemy, activates the ability by class, and
 * asserts the enemy's GAS Health dropped — proving a generated ability runs in
 * the real engine. This is the functional-test gate the catalog lifecycle's
 * `verified` transition depends on.
 */
UCLASS()
class POF_API AVSAbility09Test : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AVSAbility09Test();

	virtual void PrepareTest() override;
	virtual void StartTest() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	float PhaseTime = 0.f;
	float EnemyStartHealth = 0.f;
	bool bActivated = false;
	TWeakObjectPtr<class AARPGEnemyCharacter> Enemy;
	TWeakObjectPtr<class AARPGPlayerCharacter> Player;
};

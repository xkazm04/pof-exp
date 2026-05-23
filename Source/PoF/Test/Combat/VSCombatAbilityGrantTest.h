#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSCombatAbilityGrantTest.generated.h"

/**
 * Asserts the player ASC has abilities granted on possession, and specifically
 * that GA_MeleeAttack (or a Blueprint subclass of it) is among them. Forward-compatible
 * with sub-project (2)'s ability-roster expansion.
 */
UCLASS()
class POF_API AVSCombatAbilityGrantTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AVSCombatAbilityGrantTest();

	virtual void StartTest() override;
};

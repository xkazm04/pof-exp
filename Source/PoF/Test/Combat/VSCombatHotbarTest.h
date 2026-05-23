#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSCombatHotbarTest.generated.h"

class AARPGPlayerCharacter;

/**
 * Hotbar wiring: with the player granted the roster and AbilityLoadout populated,
 * each slot 0..3 activates its ability and GA_Dodge activates via its tag.
 * Scope is activation (commit succeeds), not visible gray-box effect.
 */
UCLASS()
class POF_API AVSCombatHotbarTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AVSCombatHotbarTest();

	virtual void PrepareTest() override;
	virtual void StartTest() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	TWeakObjectPtr<AARPGPlayerCharacter> Player;

	int32 Phase = 0;
	float PhaseTime = 0.f;
	int32 SlotIndex = 0;
};

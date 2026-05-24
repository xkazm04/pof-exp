#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSCombatTwoHealthSystemsTest.generated.h"

class AARPGPlayerCharacter;

/**
 * Guards the folder-03 §5 resolution: AARPGPlayerCharacter's plain-float Health
 * getters now read GAS (UARPGAttributeSet). Asserts GetHealth()/GetMaxHealth()
 * equal the GAS attribute values — i.e. the two Health systems no longer drift.
 */
UCLASS()
class POF_API AVSCombatTwoHealthSystemsTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AVSCombatTwoHealthSystemsTest();

	virtual void PrepareTest() override;
	virtual void StartTest() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	TWeakObjectPtr<AARPGPlayerCharacter> Player;
	float PhaseTime = 0.f;
	bool bAsserted = false;
};

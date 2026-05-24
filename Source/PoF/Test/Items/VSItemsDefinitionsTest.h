#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSItemsDefinitionsTest.generated.h"

/**
 * Folder-09 Items catalog gate.
 *
 * Validates the UARPGItemDefinition schema by instantiating a transient
 * item definition, setting representative properties, and asserting they
 * roundtrip + the rarity helpers behave. Pure runtime — no .uasset is
 * touched, so this test is robust against catalog churn. This is the
 * per-section verify gate the items recipe depends on.
 *
 * Uses the proven Phase C tick-gated pattern: setup in StartTest, run
 * assertions + FinishTest on the first Tick (≥0.2s) so the AFunctionalTest
 * framework is fully initialized.
 */
UCLASS()
class POF_API AVSItemsDefinitionsTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AVSItemsDefinitionsTest();

	virtual void StartTest() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	float PhaseTime = 0.f;
	bool bDone = false;
};

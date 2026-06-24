#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSInventoryPotionTest.generated.h"

/**
 * Stream 4 (Inventory) potion gate.
 *
 * Verifies the authored Minor Health Potion (app entity item-7) + its heal effect:
 *   - /Game/Inventory/DA_HealthPotion loads, Type=Consumable, stackable, OnUseEffect set.
 *   - OnUseEffect (UGE_HealthPotion) carries an additive IncomingHeal modifier with a
 *     static magnitude of +50 — the config gate behind the 50->100 runtime heal proof
 *     (shots/inventory/inv-loot-heal.json, verified by the Observation Spine harness).
 *
 * Mirrors VSItemsDefinitionsTest's Phase-C tick-gated pattern (setup in StartTest,
 * assertions + FinishTest on the first Tick >=0.2s).
 */
UCLASS()
class POF_API AVSInventoryPotionTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AVSInventoryPotionTest();

	virtual void StartTest() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	float PhaseTime = 0.f;
	bool bDone = false;
};

#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSItemsDefinitionsTest.generated.h"

/**
 * Folder-09 Items catalog gate.
 *
 * Two-part gate:
 *   1. Schema — instantiate a transient UARPGItemDefinition, set representative
 *      properties, assert they roundtrip + the rarity helpers behave.
 *   2. Real asset — load /Game/Data/Items/DA_IronLongsword (the catalog target,
 *      entity item-1, authored by Content/Python/author_items.py) and assert its
 *      canonical fields + that its OnEquipEffect grants +15 AttackPower. This
 *      closes the "fixture, not the entity" gap: the gate verifies the asset the
 *      pipeline actually produces, not just the schema. This is the per-section
 *      verify gate the items recipe depends on.
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

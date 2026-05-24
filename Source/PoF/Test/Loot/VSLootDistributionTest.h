#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSLootDistributionTest.generated.h"

/**
 * Folder-09 Loot catalog gate.
 *
 * Validates the UARPGLootTable.RollLoot API by building a transient
 * single-entry table (NothingWeight=0 ⇒ deterministic 100% drop), rolling
 * N times, and asserting that every roll returns exactly one instance of
 * the configured drop. Pure runtime — no .uasset is touched. This is the
 * per-section verify gate the loot-tables recipe depends on.
 *
 * Uses the proven Phase C tick-gated pattern: setup in StartTest, run the
 * rolls + FinishTest on the first Tick (≥0.2s) so the AFunctionalTest
 * framework is fully initialized.
 */
UCLASS()
class POF_API AVSLootDistributionTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AVSLootDistributionTest();

	virtual void StartTest() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	/** Number of weighted rolls to perform. Higher = more confidence; 200 is fast and decisive. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VSLoot|Roll", meta = (ClampMin = "20"))
	int32 RollCount = 200;

	/** Minimum success rate (drops at least 1 instance per roll) required to pass. With NothingWeight=0 this is 1.0. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VSLoot|Roll", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinSuccessRate = 0.95f;

private:
	float PhaseTime = 0.f;
	bool bDone = false;
};

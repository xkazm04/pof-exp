#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_LineOfSight.generated.h"

/**
 * Scores positions by how much line-of-sight exposure they have to a threat actor.
 * Uses multiple trace rays from the test position toward the threat at varying heights
 * to estimate exposure percentage.
 *
 * Scoring:
 *  - 0.0 = fully exposed (all traces reach the threat — worst for cover)
 *  - 1.0 = fully occluded (no traces reach — best cover)
 *
 * Use with Score mode to prefer positions with low exposure, or
 * Filter mode to reject positions above a threshold.
 *
 * Designed to complement EnvQueryGenerator_CoverPositions by evaluating
 * how effective a cover position actually is.
 */
UCLASS(meta = (DisplayName = "Line of Sight Exposure"))
class POF_API UEnvQueryTest_LineOfSight : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UEnvQueryTest_LineOfSight();

	/** The threat actor whose line of sight we test against. */
	UPROPERTY(EditDefaultsOnly, Category = "Test")
	TSubclassOf<UEnvQueryContext> ThreatContext;

	/**
	 * Number of vertical trace rays to cast (spread from crouch to standing height).
	 * More rays = more accurate exposure estimate but higher cost.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Test", meta = (ClampMin = "1", ClampMax = "5"))
	int32 NumberOfTraceHeights = 3;

	/** Lowest trace height offset from position (crouch height). */
	UPROPERTY(EditDefaultsOnly, Category = "Test", meta = (ClampMin = "0"))
	float MinTraceHeight = 40.f;

	/** Highest trace height offset from position (standing height). */
	UPROPERTY(EditDefaultsOnly, Category = "Test", meta = (ClampMin = "50"))
	float MaxTraceHeight = 170.f;

	/** Trace channel for visibility checks. */
	UPROPERTY(EditDefaultsOnly, Category = "Test")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

protected:
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};

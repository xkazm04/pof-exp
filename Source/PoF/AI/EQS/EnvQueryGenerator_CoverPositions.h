#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ProjectedPoints.h"
#include "EnvQueryGenerator_CoverPositions.generated.h"

/**
 * Generates potential cover positions around a context actor by sampling
 * points near nav mesh edges and obstacles. Identifies natural cover
 * (walls, pillars, elevation changes) by tracing against level geometry
 * and filtering for positions that provide obstruction from a threat.
 *
 * Algorithm:
 *  1. Generate candidate points in an annular ring around the threat.
 *  2. For each point, trace outward from the point away from the threat
 *     to detect nearby geometry (walls, pillars).
 *  3. If geometry is found within CoverCheckDistance, push the point
 *     to the geometry's surface (the "cover side") and keep it.
 *  4. Points without nearby geometry are discarded.
 *  5. Remaining points are projected onto the nav mesh.
 *
 * Pair with EnvQueryTest_LineOfSight and EnvQueryTest_ElevationAdvantage
 * for full terrain-aware tactical positioning.
 */
UCLASS(meta = (DisplayName = "Cover Positions"))
class POF_API UEnvQueryGenerator_CoverPositions : public UEnvQueryGenerator_ProjectedPoints
{
	GENERATED_BODY()

public:
	UEnvQueryGenerator_CoverPositions();

	/** The threat actor context — cover is evaluated relative to this actor. */
	UPROPERTY(EditDefaultsOnly, Category = "Generator")
	TSubclassOf<UEnvQueryContext> ThreatContext;

	/** Number of candidate sample points around the threat. */
	UPROPERTY(EditDefaultsOnly, Category = "Generator", meta = (ClampMin = "8", ClampMax = "72"))
	int32 SampleCount = 36;

	/** Minimum search radius from the threat (UU). */
	UPROPERTY(EditDefaultsOnly, Category = "Generator", meta = (ClampMin = "100"))
	float MinRadius = 300.f;

	/** Maximum search radius from the threat (UU). */
	UPROPERTY(EditDefaultsOnly, Category = "Generator", meta = (ClampMin = "200"))
	float MaxRadius = 1200.f;

	/** Number of radial rings to sample between MinRadius and MaxRadius. */
	UPROPERTY(EditDefaultsOnly, Category = "Generator", meta = (ClampMin = "1", ClampMax = "5"))
	int32 NumberOfRings = 3;

	/**
	 * Max distance from a candidate point to geometry to qualify as cover.
	 * Points further than this from any obstacle are discarded.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Generator", meta = (ClampMin = "50"))
	float CoverCheckDistance = 150.f;

	/** Trace channel used for cover geometry detection. */
	UPROPERTY(EditDefaultsOnly, Category = "Generator")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_WorldStatic;

	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};

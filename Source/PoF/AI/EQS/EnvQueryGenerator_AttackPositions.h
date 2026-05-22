#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ProjectedPoints.h"
#include "EnvQueryGenerator_AttackPositions.generated.h"

/**
 * Generates points in a ring around a context actor (typically TargetActor)
 * at a configurable melee attack distance.
 * Used for both FindAttackPosition and FindFlankPosition queries —
 * the scoring (front vs. flank) is done by separate EQS tests.
 */
UCLASS(meta = (DisplayName = "Attack Ring Positions"))
class POF_API UEnvQueryGenerator_AttackPositions : public UEnvQueryGenerator_ProjectedPoints
{
	GENERATED_BODY()

public:
	UEnvQueryGenerator_AttackPositions();

	/** The context around which to generate the ring (e.g., TargetActor). */
	UPROPERTY(EditDefaultsOnly, Category = "Generator")
	TSubclassOf<UEnvQueryContext> CenterContext;

	/** Distance from the center actor at which to generate points. */
	UPROPERTY(EditDefaultsOnly, Category = "Generator", meta = (ClampMin = "50"))
	float AttackDistance = 200.f;

	/** Number of points evenly distributed around the ring. */
	UPROPERTY(EditDefaultsOnly, Category = "Generator", meta = (ClampMin = "4", ClampMax = "36"))
	int32 NumberOfPoints = 12;

	/**
	 * When true, adds a second inner ring at half AttackDistance.
	 * Useful for finding closer positions if the outer ring is obstructed.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Generator")
	bool bGenerateInnerRing = false;

	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};

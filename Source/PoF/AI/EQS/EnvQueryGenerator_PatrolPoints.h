#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ProjectedPoints.h"
#include "EnvQueryGenerator_PatrolPoints.generated.h"

/**
 * Generates random navigable points around the querier for patrol behavior.
 * Projects all generated points onto the nav mesh.
 */
UCLASS(meta = (DisplayName = "Patrol Points"))
class POF_API UEnvQueryGenerator_PatrolPoints : public UEnvQueryGenerator_ProjectedPoints
{
	GENERATED_BODY()

public:
	UEnvQueryGenerator_PatrolPoints();

	/** Number of random points to generate. */
	UPROPERTY(EditDefaultsOnly, Category = "Generator", meta = (ClampMin = "1", ClampMax = "50"))
	int32 NumberOfPoints = 15;

	/** Minimum distance from the querier. */
	UPROPERTY(EditDefaultsOnly, Category = "Generator", meta = (ClampMin = "0"))
	float MinRadius = 500.f;

	/** Maximum distance from the querier. */
	UPROPERTY(EditDefaultsOnly, Category = "Generator", meta = (ClampMin = "100"))
	float MaxRadius = 1500.f;

	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};

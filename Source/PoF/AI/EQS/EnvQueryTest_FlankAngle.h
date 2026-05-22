#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_FlankAngle.generated.h"

/**
 * Scores positions based on the angle between the target's forward vector
 * and the direction from the target to the test position.
 *
 * - 0° = directly in front of the target (worst for flanking)
 * - 180° = directly behind (best for flanking)
 * - 90°/270° = to the sides
 *
 * Returns the absolute angle in degrees [0-180].
 * Use with Filter/Score to prefer behind (high angle) or front (low angle).
 */
UCLASS(meta = (DisplayName = "Flank Angle"))
class POF_API UEnvQueryTest_FlankAngle : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UEnvQueryTest_FlankAngle();

	/** The context actor whose forward direction is used (typically TargetActor/player). */
	UPROPERTY(EditDefaultsOnly, Category = "Test")
	TSubclassOf<UEnvQueryContext> TargetContext;

protected:
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};

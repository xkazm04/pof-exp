#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_ElevationAdvantage.generated.h"

/**
 * Scores positions by their elevation relative to a reference context (typically the threat).
 * Higher positions receive better scores — simulates the tactical advantage of high ground.
 *
 * Score output:
 *  - Positive elevation difference = positive score (high ground advantage)
 *  - Zero or negative = 0 (no advantage or disadvantage)
 *
 * The score is clamped: height differences beyond MaxElevationBonus are capped at 1.0.
 * This prevents extreme heights from dominating the score unreasonably.
 *
 * Designed to work with cover queries — AI will prefer elevated cover positions
 * (ledges, raised platforms, staircases) over flat ones.
 */
UCLASS(meta = (DisplayName = "Elevation Advantage"))
class POF_API UEnvQueryTest_ElevationAdvantage : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UEnvQueryTest_ElevationAdvantage();

	/** Context actor to measure elevation against (typically the threat/player). */
	UPROPERTY(EditDefaultsOnly, Category = "Test")
	TSubclassOf<UEnvQueryContext> ReferenceContext;

	/**
	 * The elevation difference (in UU) that maps to a score of 1.0.
	 * Differences beyond this are clamped to 1.0.
	 * E.g., 300 = 3 meters of height advantage gives max score.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Test", meta = (ClampMin = "50"))
	float MaxElevationBonus = 300.f;

	/**
	 * When true, negative elevation (lower ground) receives a penalty score.
	 * When false, lower ground scores 0 (no bonus, no penalty).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Test")
	bool bPenalizeLowGround = false;

protected:
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};

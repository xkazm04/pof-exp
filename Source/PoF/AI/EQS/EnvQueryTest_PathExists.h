#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvQueryTest_PathExists.generated.h"

/**
 * Tests whether a valid navigation path exists from the querier to each item.
 * Returns 1.0 if reachable, 0.0 if not.
 * Use as a filter to remove unreachable positions from attack/flank queries.
 */
UCLASS(meta = (DisplayName = "Path Exists To Querier"))
class POF_API UEnvQueryTest_PathExists : public UEnvQueryTest
{
	GENERATED_BODY()

public:
	UEnvQueryTest_PathExists();

	/** Context to test path from (default: Querier). */
	UPROPERTY(EditDefaultsOnly, Category = "Test")
	TSubclassOf<UEnvQueryContext> PathFromContext;

protected:
	virtual void RunTest(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;
};

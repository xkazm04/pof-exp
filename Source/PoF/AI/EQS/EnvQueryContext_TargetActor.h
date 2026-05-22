#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvQueryContext_TargetActor.generated.h"

/**
 * EQS context that resolves to the TargetActor blackboard key.
 * Used by attack and flank queries as the reference point around
 * which to generate and score positions.
 */
UCLASS()
class POF_API UEnvQueryContext_TargetActor : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_BossPhase.generated.h"

/**
 * BT decorator that checks the boss's current phase.
 * Use to branch boss behavior based on phase index.
 */
UCLASS()
class POF_API UBTDecorator_BossPhase : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_BossPhase();

	virtual FString GetStaticDescription() const override;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	/** Minimum phase index for this condition to be true (inclusive). */
	UPROPERTY(EditAnywhere, Category = "Boss", meta = (ClampMin = "0"))
	int32 MinPhase = 0;

	/** Maximum phase index (inclusive). -1 = no upper bound. */
	UPROPERTY(EditAnywhere, Category = "Boss")
	int32 MaxPhase = -1;
};

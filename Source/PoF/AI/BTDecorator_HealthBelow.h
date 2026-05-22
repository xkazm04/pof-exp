#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HealthBelow.generated.h"

/**
 * Checks whether the AI pawn's health is below a threshold percentage.
 * Used to gate retreat and heal behaviors in the behavior tree.
 */
UCLASS()
class POF_API UBTDecorator_HealthBelow : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_HealthBelow();

	/** Health threshold as a fraction (0.0 - 1.0). Returns true if health ratio < this value. */
	UPROPERTY(EditAnywhere, Category = "Health", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HealthThreshold = 0.3f;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
};

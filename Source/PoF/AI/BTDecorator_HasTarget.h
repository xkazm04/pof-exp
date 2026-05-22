#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_HasTarget.generated.h"

/**
 * Checks whether the TargetActor blackboard key is set (non-null).
 * Used to gate the Chase and Attack branches.
 */
UCLASS()
class POF_API UBTDecorator_HasTarget : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_HasTarget();

protected:
	/** Blackboard key for the target actor. */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};

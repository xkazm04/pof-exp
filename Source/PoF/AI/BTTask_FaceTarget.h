#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FaceTarget.generated.h"

/**
 * Rotates the pawn to face the TargetActor blackboard key.
 * Completes immediately after setting the focus (AIController handles rotation).
 */
UCLASS()
class POF_API UBTTask_FaceTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FaceTarget();

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};

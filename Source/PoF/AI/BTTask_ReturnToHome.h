#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ReturnToHome.generated.h"

/**
 * Moves the AI pawn back to its HomeLocation blackboard key.
 * Used after aggro is dropped (leash, timeout) to return to patrol origin.
 */
UCLASS()
class POF_API UBTTask_ReturnToHome : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ReturnToHome();

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector HomeLocationKey;

	/** Acceptance radius — how close to home before the task succeeds. */
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "10"))
	float AcceptanceRadius = 100.f;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};

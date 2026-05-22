#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindPatrolPoint.generated.h"

class UEnvQuery;

/**
 * Finds a random navigable patrol point and writes it to the PatrolLocation blackboard key.
 * Can use an optional EQS query for smarter point selection, or falls back to
 * UNavigationSystemV1::GetRandomReachablePointInRadius.
 */
UCLASS()
class POF_API UBTTask_FindPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindPatrolPoint();

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PatrolLocationKey;

	/** Radius around the pawn to search for a random patrol point. */
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (ClampMin = "100"))
	float PatrolRadius = 1500.f;

	/**
	 * Optional EQS query for patrol point selection.
	 * If set, runs the EQS query and picks the best result.
	 * If null, uses random navigable point in radius.
	 */
	UPROPERTY(EditAnywhere, Category = "Patrol")
	TObjectPtr<UEnvQuery> PatrolQuery;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

private:
	/** Fallback: random navigable point in radius. */
	EBTNodeResult::Type FindRandomPoint(UBehaviorTreeComponent& OwnerComp);

	/** EQS-based point selection. */
	EBTNodeResult::Type RunEQSQuery(UBehaviorTreeComponent& OwnerComp);

	void OnEQSQueryFinished(TSharedPtr<struct FEnvQueryResult> Result, UBehaviorTreeComponent* OwnerComp);
};

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Navigation/PathFollowingComponent.h"
#include "BTTask_MaintainDistance.generated.h"

struct FBTMaintainDistanceMemory
{
	// No per-instance state needed — we use WaitForMessage
};

/**
 * Moves the AI pawn to maintain a preferred distance from the target.
 * If the target is closer than RetreatDistance, the AI moves away.
 * If the target is farther than PreferredDistance, the AI moves closer.
 * Used by Ranged Caster archetype to kite the player.
 */
UCLASS()
class POF_API UBTTask_MaintainDistance : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MaintainDistance();

	/** Blackboard key for the target actor. */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	/** Blackboard key for preferred combat distance. */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector PreferredDistanceKey;

	/** Blackboard key for retreat distance. */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector RetreatDistanceKey;

	/** Acceptance radius for the move request. */
	UPROPERTY(EditAnywhere, Category = "Movement", meta = (ClampMin = "10"))
	float AcceptanceRadius = 50.f;

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTMaintainDistanceMemory); }

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};

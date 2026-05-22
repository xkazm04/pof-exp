#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateCombatState.generated.h"

/** Per-instance memory for BTService_UpdateCombatState (services are shared singletons). */
struct FBTUpdateCombatStateMemory
{
	float NoLOSTimer = 0.f;
};

/**
 * Ticking service that keeps combat-related blackboard keys up to date:
 *  - DistanceToTarget: live distance to the current target
 *  - HasLineOfSight: updated LOS check (trace-based, not just perception)
 *  - Clears TargetActor if the target has died
 *  - Clears TargetActor if the AI has exceeded leash distance from home
 *  - Clears TargetActor after aggro timeout without LOS
 *
 * Attach this service to the root of the behavior tree.
 */
UCLASS()
class POF_API UBTService_UpdateCombatState : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateCombatState();

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector DistanceToTargetKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector HasLineOfSightKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector HomeLocationKey;

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual uint16 GetInstanceMemorySize() const override;
};

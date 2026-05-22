#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_SquadCoordination.generated.h"

class UARPGSquadManager;

/** Per-instance memory for squad coordination service. */
struct FBTSquadCoordinationMemory
{
	TWeakObjectPtr<UARPGSquadManager> CachedSquadManager;
};

/**
 * BT Service that integrates with the squad manager for coordinated combat:
 * - Writes a "CanAttack" bool to blackboard based on attack token availability
 * - Writes a "FlankPosition" vector for the enemy's assigned flank position
 * - Syncs focus target from squad manager to blackboard
 */
UCLASS()
class POF_API UBTService_SquadCoordination : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_SquadCoordination();

	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTSquadCoordinationMemory); }

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector CanAttackKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector FlankPositionKey;

	/** Distance from target for flanking positions. */
	UPROPERTY(EditAnywhere, Category = "Squad", meta = (ClampMin = "100"))
	float FlankDistance = 400.f;

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};

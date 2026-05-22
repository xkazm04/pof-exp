#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_IsInAttackRange.generated.h"

/**
 * Checks whether the pawn is within attack range of TargetActor.
 * Can read AttackRange from a blackboard key (preferred — set by enemy archetype)
 * or use a fallback hardcoded value.
 */
UCLASS()
class POF_API UBTDecorator_IsInAttackRange : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_IsInAttackRange();

protected:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

	/** Blackboard key for attack range. If set, overrides the hardcoded AttackRange. */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector AttackRangeKey;

	/** Fallback attack range used when the blackboard key is not set or zero. */
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0"))
	float FallbackAttackRange = 200.f;

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};

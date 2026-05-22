#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "STEvaluator_CombatData.generated.h"

/**
 * Instance data for CombatData evaluator.
 * Output properties are bound to other nodes via Live Property Binding.
 */
USTRUCT()
struct FSTEvaluator_CombatDataInstanceData
{
	GENERATED_BODY()

	/** [Output] Current target actor (resolved from perception). */
	UPROPERTY(EditAnywhere, Category = "Output")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** [Output] Distance to current target. */
	UPROPERTY(EditAnywhere, Category = "Output")
	float DistanceToTarget = 0.f;

	/** [Output] Whether we have line of sight to target. */
	UPROPERTY(EditAnywhere, Category = "Output")
	bool bHasLineOfSight = false;

	/** [Output] Current health ratio (0-1). */
	UPROPERTY(EditAnywhere, Category = "Output")
	float HealthRatio = 1.f;

	/** [Output] Whether the AI is in attack range. */
	UPROPERTY(EditAnywhere, Category = "Output")
	bool bInAttackRange = false;

	/** Attack range threshold. */
	UPROPERTY(EditAnywhere, Category = "Config")
	float AttackRange = 200.f;
};

/**
 * State Tree evaluator that gathers combat-relevant data each tick.
 * Reads from the AI controller's blackboard and ASC.
 * Output properties can be bound to conditions and tasks for decision-making.
 */
USTRUCT(meta = (DisplayName = "Combat Data Evaluator", Category = "ARPG|AI"))
struct POF_API FSTEvaluator_CombatData : public FStateTreeEvaluatorBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTEvaluator_CombatDataInstanceData;

	FSTEvaluator_CombatData() = default;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

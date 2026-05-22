#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "STCondition_HasTarget.generated.h"

/**
 * Instance data for HasTarget condition.
 */
USTRUCT()
struct FSTCondition_HasTargetInstanceData
{
	GENERATED_BODY()

	/** The target actor to check. Bindable via State Tree property binding. */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<AActor> TargetActor = nullptr;

	/** If true, also checks that the target is alive (must implement IsDead). */
	UPROPERTY(EditAnywhere, Category = "Input")
	bool bCheckAlive = true;
};

/**
 * State Tree condition that checks if a target actor is set (and optionally alive).
 * Used for state transitions between idle/patrol and combat states.
 */
USTRUCT(meta = (DisplayName = "Has Target", Category = "ARPG|AI"))
struct POF_API FSTCondition_HasTarget : public FStateTreeConditionBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTCondition_HasTargetInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

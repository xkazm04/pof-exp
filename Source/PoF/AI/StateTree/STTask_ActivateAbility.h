#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "GameplayTagContainer.h"
#include "STTask_ActivateAbility.generated.h"

class UAbilitySystemComponent;

/**
 * Instance data for the ActivateAbility StateTree task.
 * State Tree uses separate instance data structs (not member variables on the task).
 */
USTRUCT()
struct FSTTask_ActivateAbilityInstanceData
{
	GENERATED_BODY()

	/** The ability tag to activate. Bindable from state tree properties. */
	UPROPERTY(EditAnywhere, Category = "Input")
	FGameplayTag AbilityTag;

	/** Cached ASC resolved on EnterState. */
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	/** Whether the ability was successfully activated. */
	bool bActivated = false;
};

/**
 * State Tree task that activates a gameplay ability by tag on the AI pawn.
 * Succeeds immediately after activation (fire-and-forget).
 * Designed for State Tree AI as a 5.7+ alternative to BT tasks.
 */
USTRUCT(meta = (DisplayName = "Activate Ability", Category = "ARPG|AI"))
struct POF_API FSTTask_ActivateAbility : public FStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FSTTask_ActivateAbilityInstanceData;

	FSTTask_ActivateAbility()
	{
		bShouldCallTick = false;
	}

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

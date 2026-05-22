#include "AI/StateTree/STTask_ActivateAbility.h"
#include "StateTreeExecutionContext.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AIController.h"

EStateTreeRunStatus FSTTask_ActivateAbility::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);
	InstanceData.bActivated = false;
	InstanceData.CachedASC = nullptr;

	if (!InstanceData.AbilityTag.IsValid())
	{
		return EStateTreeRunStatus::Failed;
	}

	// Resolve the pawn's ASC
	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	const APawn* Pawn = nullptr;

	if (const AAIController* AIC = Cast<AAIController>(Owner))
	{
		Pawn = AIC->GetPawn();
	}
	else
	{
		Pawn = Cast<APawn>(Owner);
	}

	if (!Pawn)
	{
		return EStateTreeRunStatus::Failed;
	}

	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn);
	UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.CachedASC = ASC;

	// Activate by tag
	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(InstanceData.AbilityTag);

	if (ASC->TryActivateAbilitiesByTag(TagContainer))
	{
		InstanceData.bActivated = true;
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Failed;
}

void FSTTask_ActivateAbility::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// If we're leaving the state and the ability was activated, cancel it
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	if (InstanceData.bActivated && InstanceData.CachedASC.IsValid())
	{
		FGameplayTagContainer TagContainer;
		TagContainer.AddTag(InstanceData.AbilityTag);
		InstanceData.CachedASC->CancelAbilities(&TagContainer);
	}

	InstanceData.bActivated = false;
	InstanceData.CachedASC = nullptr;
}

#include "AI/BTTask_ExecuteAttackAbility.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/ARPGGameplayTags.h"

UBTTask_ExecuteAttackAbility::UBTTask_ExecuteAttackAbility()
{
	NodeName = TEXT("Execute Attack Ability");
	bNotifyTaskFinished = true;

	// Default to melee light attack
	AbilityTag = ARPGGameplayTags::Ability_Melee_LightAttack;
}

EBTNodeResult::Type UBTTask_ExecuteAttackAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTAttackAbilityMemory* Memory = reinterpret_cast<FBTAttackAbilityMemory*>(NodeMemory);
	*Memory = FBTAttackAbilityMemory();

	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn);
	UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return EBTNodeResult::Failed;
	}

	// Try to activate the ability by tag
	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(AbilityTag);

	// Find the ability spec handle before activation so we can track it
	TArray<FGameplayAbilitySpec*> MatchingSpecs;
	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(TagContainer, MatchingSpecs);
	if (MatchingSpecs.IsEmpty())
	{
		return EBTNodeResult::Failed;
	}

	Memory->ActivatedAbilityHandle = MatchingSpecs[0]->Handle;

	if (!ASC->TryActivateAbilitiesByTag(TagContainer))
	{
		return EBTNodeResult::Failed;
	}

	Memory->bAbilityActivated = true;

	// Listen for the ability ending — filter to only OUR ability's spec handle
	Memory->AbilityEndedHandle = ASC->OnAbilityEnded.AddLambda(
		[this, &OwnerComp, NodeMemory](const FAbilityEndedData& Data)
		{
			FBTAttackAbilityMemory* Mem = reinterpret_cast<FBTAttackAbilityMemory*>(NodeMemory);
			if (Data.AbilitySpecHandle == Mem->ActivatedAbilityHandle)
			{
				OnAbilityEnded(&OwnerComp, NodeMemory);
			}
		}
	);

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_ExecuteAttackAbility::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTAttackAbilityMemory* Memory = reinterpret_cast<FBTAttackAbilityMemory*>(NodeMemory);

	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;

	if (Pawn)
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn))
		{
			if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
			{
				// Cancel the ability and remove listener
				FGameplayTagContainer TagContainer;
				TagContainer.AddTag(AbilityTag);
				ASC->CancelAbilities(&TagContainer);

				ASC->OnAbilityEnded.Remove(Memory->AbilityEndedHandle);
			}
		}
	}

	return EBTNodeResult::Aborted;
}

void UBTTask_ExecuteAttackAbility::OnAbilityEnded(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	if (!OwnerComp)
	{
		return;
	}

	FBTAttackAbilityMemory* Memory = reinterpret_cast<FBTAttackAbilityMemory*>(NodeMemory);

	// Clean up the delegate
	AAIController* AIC = OwnerComp->GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (Pawn)
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn))
		{
			if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
			{
				ASC->OnAbilityEnded.Remove(Memory->AbilityEndedHandle);
			}
		}
	}

	FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
}

FString UBTTask_ExecuteAttackAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("Attack: %s"), *AbilityTag.ToString());
}

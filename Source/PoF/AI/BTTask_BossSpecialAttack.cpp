#include "AI/BTTask_BossSpecialAttack.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UBTTask_BossSpecialAttack::UBTTask_BossSpecialAttack()
{
	NodeName = TEXT("Boss Special Attack");
	bCreateNodeInstance = true;
}

uint16 UBTTask_BossSpecialAttack::GetInstanceMemorySize() const
{
	return sizeof(FBTBossAttackMemory);
}

EBTNodeResult::Type UBTTask_BossSpecialAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn) return EBTNodeResult::Failed;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn);
	if (!ASI) return EBTNodeResult::Failed;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return EBTNodeResult::Failed;

	if (!AbilityTag.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	// Try to activate ability by tag
	bool bActivated = ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityTag));
	if (!bActivated)
	{
		return EBTNodeResult::Failed;
	}

	if (!bWaitForEnd)
	{
		return EBTNodeResult::Succeeded;
	}

	// Wait for the ability to end
	FBTBossAttackMemory* Memory = reinterpret_cast<FBTBossAttackMemory*>(NodeMemory);
	new (Memory) FBTBossAttackMemory();
	Memory->bAbilityEnded = false;

	// Find the active ability spec handle
	TArray<FGameplayAbilitySpec*> MatchingSpecs;
	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(FGameplayTagContainer(AbilityTag), MatchingSpecs);
	for (FGameplayAbilitySpec* Spec : MatchingSpecs)
	{
		if (Spec->IsActive())
		{
			Memory->ActiveAbilityHandle = Spec->Handle;
			break;
		}
	}

	Memory->AbilityEndedHandle = ASC->OnAbilityEnded.AddLambda(
		[Memory, &OwnerComp](const FAbilityEndedData& EndedData)
		{
			if (EndedData.AbilitySpecHandle == Memory->ActiveAbilityHandle)
			{
				Memory->bAbilityEnded = true;
				OwnerComp.OnTaskFinished(Cast<UBTTaskNode>(OwnerComp.GetActiveNode()), EBTNodeResult::Succeeded);
			}
		});

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_BossSpecialAttack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTBossAttackMemory* Memory = reinterpret_cast<FBTBossAttackMemory*>(NodeMemory);

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (AIC)
	{
		APawn* Pawn = AIC->GetPawn();
		if (Pawn)
		{
			IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn);
			if (ASI)
			{
				UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
				if (ASC && Memory->AbilityEndedHandle.IsValid())
				{
					ASC->OnAbilityEnded.Remove(Memory->AbilityEndedHandle);
				}
			}
		}
	}

	return EBTNodeResult::Aborted;
}

FString UBTTask_BossSpecialAttack::GetStaticDescription() const
{
	return FString::Printf(TEXT("Activate: %s%s"),
		*AbilityTag.ToString(),
		bWaitForEnd ? TEXT(" (wait)") : TEXT(""));
}

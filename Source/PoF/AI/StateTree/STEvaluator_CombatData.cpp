#include "AI/StateTree/STEvaluator_CombatData.h"
#include "StateTreeExecutionContext.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AI/ARPGAIController.h"

void FSTEvaluator_CombatData::TreeStart(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& Data = Context.GetInstanceData<FInstanceDataType>(*this);
	Data.TargetActor = nullptr;
	Data.DistanceToTarget = 0.f;
	Data.bHasLineOfSight = false;
	Data.HealthRatio = 1.f;
	Data.bInAttackRange = false;
}

void FSTEvaluator_CombatData::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData<FInstanceDataType>(*this);

	const AActor* Owner = Cast<AActor>(Context.GetOwner());
	const AAIController* AIC = Cast<AAIController>(Owner);
	const APawn* Pawn = AIC ? AIC->GetPawn() : Cast<APawn>(Owner);

	if (!Pawn)
	{
		return;
	}

	// If we have an AIC, read from blackboard
	if (!AIC && Pawn)
	{
		AIC = Cast<AAIController>(Pawn->GetController());
	}

	if (AIC)
	{
		const UBlackboardComponent* BBComp = AIC->GetBlackboardComponent();
		if (BBComp)
		{
			Data.TargetActor = Cast<AActor>(BBComp->GetValueAsObject(AARPGAIController::KEY_TargetActor));
			Data.DistanceToTarget = BBComp->GetValueAsFloat(AARPGAIController::KEY_DistanceToTarget);
			Data.bHasLineOfSight = BBComp->GetValueAsBool(AARPGAIController::KEY_HasLineOfSight);

			const float BBRange = BBComp->GetValueAsFloat(AARPGAIController::KEY_AttackRange);
			if (BBRange > 0.f)
			{
				Data.AttackRange = BBRange;
			}
		}
	}

	// Check attack range
	Data.bInAttackRange = (Data.TargetActor != nullptr && Data.DistanceToTarget <= Data.AttackRange);

	// Health ratio from ASC
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn))
	{
		if (const UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			const UARPGAttributeSet* Attrs = ASC->GetSet<UARPGAttributeSet>();
			if (Attrs && Attrs->GetMaxHealth() > 0.f)
			{
				Data.HealthRatio = Attrs->GetHealth() / Attrs->GetMaxHealth();
			}
		}
	}
}

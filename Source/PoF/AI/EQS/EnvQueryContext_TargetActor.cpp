#include "AI/EQS/EnvQueryContext_TargetActor.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

void UEnvQueryContext_TargetActor::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	// Get the querier (the AI pawn)
	const UObject* QuerierObject = QueryInstance.Owner.Get();
	if (!QuerierObject)
	{
		return;
	}

	// The querier can be the pawn or the controller — handle both
	const AActor* QuerierActor = Cast<AActor>(QuerierObject);
	if (!QuerierActor)
	{
		return;
	}

	const APawn* QuerierPawn = Cast<APawn>(QuerierActor);
	const AAIController* AIC = QuerierPawn ? Cast<AAIController>(QuerierPawn->GetController()) : Cast<AAIController>(QuerierActor);
	if (!AIC)
	{
		return;
	}

	const UBlackboardComponent* BBComp = AIC->GetBlackboardComponent();
	if (!BBComp)
	{
		return;
	}

	AActor* TargetActor = Cast<AActor>(BBComp->GetValueAsObject(TEXT("TargetActor")));
	if (TargetActor)
	{
		UEnvQueryItemType_Actor::SetContextHelper(ContextData, TargetActor);
	}
}

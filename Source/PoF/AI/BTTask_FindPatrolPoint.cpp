#include "AI/BTTask_FindPatrolPoint.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/EnvQuery.h"

UBTTask_FindPatrolPoint::UBTTask_FindPatrolPoint()
{
	NodeName = TEXT("Find Patrol Point");

	PatrolLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FindPatrolPoint, PatrolLocationKey));
	PatrolLocationKey.SelectedKeyName = TEXT("PatrolLocation");
}

void UBTTask_FindPatrolPoint::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		PatrolLocationKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UBTTask_FindPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (PatrolQuery)
	{
		return RunEQSQuery(OwnerComp);
	}

	return FindRandomPoint(OwnerComp);
}

EBTNodeResult::Type UBTTask_FindPatrolPoint::FindRandomPoint(UBehaviorTreeComponent& OwnerComp)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(Pawn->GetWorld());
	if (!NavSys)
	{
		return EBTNodeResult::Failed;
	}

	FNavLocation ResultLocation;
	if (NavSys->GetRandomReachablePointInRadius(Pawn->GetActorLocation(), PatrolRadius, ResultLocation))
	{
		UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
		if (BBComp)
		{
			BBComp->SetValue<UBlackboardKeyType_Vector>(PatrolLocationKey.GetSelectedKeyID(), ResultLocation.Location);
			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTTask_FindPatrolPoint::RunEQSQuery(UBehaviorTreeComponent& OwnerComp)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	UEnvQueryManager* EQSManager = UEnvQueryManager::GetCurrent(Pawn->GetWorld());
	if (!EQSManager)
	{
		return EBTNodeResult::Failed;
	}

	FEnvQueryRequest QueryRequest(PatrolQuery, Pawn);
	QueryRequest.Execute(
		EEnvQueryRunMode::RandomBest25Pct,
		FQueryFinishedSignature::CreateUObject(this, &UBTTask_FindPatrolPoint::OnEQSQueryFinished, &OwnerComp)
	);

	return EBTNodeResult::InProgress;
}

void UBTTask_FindPatrolPoint::OnEQSQueryFinished(TSharedPtr<FEnvQueryResult> Result, UBehaviorTreeComponent* OwnerComp)
{
	if (!OwnerComp)
	{
		return;
	}

	if (!Result.IsValid() || !Result->IsSuccessful())
	{
		FinishLatentTask(*OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const FVector BestLocation = Result->GetItemAsLocation(0);

	UBlackboardComponent* BBComp = OwnerComp->GetBlackboardComponent();
	if (BBComp)
	{
		BBComp->SetValue<UBlackboardKeyType_Vector>(PatrolLocationKey.GetSelectedKeyID(), BestLocation);
		FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
	}
	else
	{
		FinishLatentTask(*OwnerComp, EBTNodeResult::Failed);
	}
}

FString UBTTask_FindPatrolPoint::GetStaticDescription() const
{
	if (PatrolQuery)
	{
		return FString::Printf(TEXT("EQS → %s (query: %s)"),
			*PatrolLocationKey.SelectedKeyName.ToString(), *PatrolQuery->GetName());
	}

	return FString::Printf(TEXT("Random Nav Point → %s (radius: %.0f)"),
		*PatrolLocationKey.SelectedKeyName.ToString(), PatrolRadius);
}

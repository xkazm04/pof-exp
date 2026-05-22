#include "AI/BTTask_MaintainDistance.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_MaintainDistance::UBTTask_MaintainDistance()
{
	NodeName = TEXT("Maintain Distance");
	bNotifyTaskFinished = true;

	// Default key names matching ARPGAIController constants
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_MaintainDistance, TargetActorKey), AActor::StaticClass());
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");

	PreferredDistanceKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_MaintainDistance, PreferredDistanceKey));
	PreferredDistanceKey.SelectedKeyName = TEXT("PreferredDistance");

	RetreatDistanceKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_MaintainDistance, RetreatDistanceKey));
	RetreatDistanceKey.SelectedKeyName = TEXT("RetreatDistance");
}

void UBTTask_MaintainDistance::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
		PreferredDistanceKey.ResolveSelectedKey(*BBAsset);
		RetreatDistanceKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UBTTask_MaintainDistance::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC)
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!BBComp)
	{
		return EBTNodeResult::Failed;
	}

	AActor* TargetActor = Cast<AActor>(BBComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!TargetActor)
	{
		return EBTNodeResult::Failed;
	}

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	const float PreferredDist = BBComp->GetValueAsFloat(PreferredDistanceKey.SelectedKeyName);
	const float RetreatDist = BBComp->GetValueAsFloat(RetreatDistanceKey.SelectedKeyName);
	const float CurrentDist = FVector::Dist(Pawn->GetActorLocation(), TargetActor->GetActorLocation());

	FVector MoveTarget;

	if (RetreatDist > 0.f && CurrentDist < RetreatDist)
	{
		// Too close — retreat away from target
		const FVector AwayDir = (Pawn->GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal();
		MoveTarget = TargetActor->GetActorLocation() + AwayDir * PreferredDist;
	}
	else if (CurrentDist > PreferredDist + AcceptanceRadius)
	{
		// Too far — move closer to preferred distance
		const FVector ToTarget = (TargetActor->GetActorLocation() - Pawn->GetActorLocation()).GetSafeNormal();
		MoveTarget = TargetActor->GetActorLocation() - ToTarget * PreferredDist;
	}
	else
	{
		// Already at preferred distance
		return EBTNodeResult::Succeeded;
	}

	// Issue the move request
	FAIMoveRequest MoveReq;
	MoveReq.SetGoalLocation(MoveTarget);
	MoveReq.SetAcceptanceRadius(AcceptanceRadius);

	const FPathFollowingRequestResult Result = AIC->MoveTo(MoveReq);

	if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
	{
		// Use the built-in message-based wait — safe and handles lifetime properly
		WaitForMessage(OwnerComp, UBrainComponent::AIMessage_MoveFinished);
		return EBTNodeResult::InProgress;
	}
	else if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTTask_MaintainDistance::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (AIC)
	{
		AIC->StopMovement();
	}

	return EBTNodeResult::Aborted;
}

FString UBTTask_MaintainDistance::GetStaticDescription() const
{
	return FString::Printf(TEXT("Maintain distance (preferred: %s, retreat: %s)"),
		*PreferredDistanceKey.SelectedKeyName.ToString(),
		*RetreatDistanceKey.SelectedKeyName.ToString());
}

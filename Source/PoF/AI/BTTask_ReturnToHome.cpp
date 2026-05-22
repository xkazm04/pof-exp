#include "AI/BTTask_ReturnToHome.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_ReturnToHome::UBTTask_ReturnToHome()
{
	NodeName = TEXT("Return To Home");

	HomeLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ReturnToHome, HomeLocationKey));
	HomeLocationKey.SelectedKeyName = TEXT("HomeLocation");
}

void UBTTask_ReturnToHome::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		HomeLocationKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UBTTask_ReturnToHome::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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

	const FVector Home = BBComp->GetValueAsVector(HomeLocationKey.SelectedKeyName);

	FAIMoveRequest MoveReq;
	MoveReq.SetGoalLocation(Home);
	MoveReq.SetAcceptanceRadius(AcceptanceRadius);

	const FPathFollowingRequestResult Result = AIC->MoveTo(MoveReq);

	if (Result.Code == EPathFollowingRequestResult::AlreadyAtGoal)
	{
		return EBTNodeResult::Succeeded;
	}
	else if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
	{
		// Use the built-in receive move completed
		WaitForMessage(OwnerComp, UBrainComponent::AIMessage_MoveFinished);
		return EBTNodeResult::InProgress;
	}

	return EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTTask_ReturnToHome::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (AIC)
	{
		AIC->StopMovement();
	}

	return EBTNodeResult::Aborted;
}

FString UBTTask_ReturnToHome::GetStaticDescription() const
{
	return FString::Printf(TEXT("Return to %s (radius: %.0f)"),
		*HomeLocationKey.SelectedKeyName.ToString(), AcceptanceRadius);
}

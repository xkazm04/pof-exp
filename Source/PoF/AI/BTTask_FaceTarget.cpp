#include "AI/BTTask_FaceTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "AIController.h"

UBTTask_FaceTarget::UBTTask_FaceTarget()
{
	NodeName = TEXT("Face Target");

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_FaceTarget, TargetActorKey), AActor::StaticClass());
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");
}

void UBTTask_FaceTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UBTTask_FaceTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!BBComp)
	{
		return EBTNodeResult::Failed;
	}

	UObject* TargetObj = BBComp->GetValue<UBlackboardKeyType_Object>(TargetActorKey.GetSelectedKeyID());
	AActor* TargetActor = Cast<AActor>(TargetObj);
	if (!TargetActor)
	{
		return EBTNodeResult::Failed;
	}

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC)
	{
		return EBTNodeResult::Failed;
	}

	// SetFocus makes the AI controller continuously rotate toward the target
	AIC->SetFocus(TargetActor);

	return EBTNodeResult::Succeeded;
}

FString UBTTask_FaceTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("Face: %s"), *TargetActorKey.SelectedKeyName.ToString());
}

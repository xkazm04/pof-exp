#include "AI/BTDecorator_HasTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

UBTDecorator_HasTarget::UBTDecorator_HasTarget()
{
	NodeName = TEXT("Has Target");

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_HasTarget, TargetActorKey), AActor::StaticClass());
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");

	// Re-evaluate when the key changes so the BT reacts immediately
	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
	FlowAbortMode = EBTFlowAbortMode::Both;
}

void UBTDecorator_HasTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}

bool UBTDecorator_HasTarget::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!BBComp)
	{
		return false;
	}

	UObject* Target = BBComp->GetValue<UBlackboardKeyType_Object>(TargetActorKey.GetSelectedKeyID());
	return Target != nullptr;
}

FString UBTDecorator_HasTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("Has Target: %s"), *TargetActorKey.SelectedKeyName.ToString());
}

#include "AI/BTDecorator_IsInAttackRange.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "AIController.h"

UBTDecorator_IsInAttackRange::UBTDecorator_IsInAttackRange()
{
	NodeName = TEXT("Is In Attack Range");

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IsInAttackRange, TargetActorKey), AActor::StaticClass());
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");

	AttackRangeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_IsInAttackRange, AttackRangeKey));
	AttackRangeKey.SelectedKeyName = TEXT("AttackRange");

	// Tick to re-evaluate distance continuously
	bNotifyTick = true;
	FlowAbortMode = EBTFlowAbortMode::Both;
}

void UBTDecorator_IsInAttackRange::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
		AttackRangeKey.ResolveSelectedKey(*BBAsset);
	}
}

bool UBTDecorator_IsInAttackRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!BBComp)
	{
		return false;
	}

	UObject* TargetObj = BBComp->GetValue<UBlackboardKeyType_Object>(TargetActorKey.GetSelectedKeyID());
	AActor* TargetActor = Cast<AActor>(TargetObj);
	if (!TargetActor)
	{
		return false;
	}

	const AAIController* AIC = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return false;
	}

	// Read attack range from blackboard; fall back to hardcoded value
	float Range = BBComp->GetValueAsFloat(AttackRangeKey.SelectedKeyName);
	if (Range <= 0.f)
	{
		Range = FallbackAttackRange;
	}

	const float Distance = FVector::Dist(Pawn->GetActorLocation(), TargetActor->GetActorLocation());
	return Distance <= Range;
}

FString UBTDecorator_IsInAttackRange::GetStaticDescription() const
{
	return FString::Printf(TEXT("In Attack Range (BB:%s, fallback:%.0f) of %s"),
		*AttackRangeKey.SelectedKeyName.ToString(), FallbackAttackRange,
		*TargetActorKey.SelectedKeyName.ToString());
}

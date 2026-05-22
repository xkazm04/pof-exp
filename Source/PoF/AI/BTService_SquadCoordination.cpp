#include "AI/BTService_SquadCoordination.h"
#include "AI/ARPGSquadManager.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Character/ARPGEnemyCharacter.h"
#include "Spawning/SpawnVolume.h"

UBTService_SquadCoordination::UBTService_SquadCoordination()
{
	NodeName = TEXT("Squad Coordination");
	Interval = 0.3f;
	RandomDeviation = 0.1f;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_SquadCoordination, TargetActorKey), AActor::StaticClass());
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");

	CanAttackKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_SquadCoordination, CanAttackKey));
	CanAttackKey.SelectedKeyName = TEXT("CanAttack");

	FlankPositionKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_SquadCoordination, FlankPositionKey));
	FlankPositionKey.SelectedKeyName = TEXT("FlankPosition");
}

void UBTService_SquadCoordination::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
		CanAttackKey.ResolveSelectedKey(*BBAsset);
		FlankPositionKey.ResolveSelectedKey(*BBAsset);
	}
}

void UBTService_SquadCoordination::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	// Try to find the squad manager from the owning spawn volume
	FBTSquadCoordinationMemory* Memory = reinterpret_cast<FBTSquadCoordinationMemory*>(NodeMemory);
	Memory->CachedSquadManager = nullptr;

	// The squad manager is discoverable via the enemy character's outer spawn volume
	// For now we'll look for it as a game instance subsystem or attached to spawn volumes
}

void UBTService_SquadCoordination::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);

	// Release any held attack token
	FBTSquadCoordinationMemory* Memory = reinterpret_cast<FBTSquadCoordinationMemory*>(NodeMemory);
	UARPGSquadManager* Squad = Memory->CachedSquadManager.Get();

	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	AARPGEnemyCharacter* Enemy = Cast<AARPGEnemyCharacter>(Pawn);

	if (Squad && Enemy)
	{
		Squad->ReleaseAttackToken(Enemy);
	}
}

void UBTService_SquadCoordination::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FBTSquadCoordinationMemory* Memory = reinterpret_cast<FBTSquadCoordinationMemory*>(NodeMemory);

	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	AARPGEnemyCharacter* Enemy = Cast<AARPGEnemyCharacter>(Pawn);
	if (!Enemy) return;

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	if (!BBComp) return;

	UARPGSquadManager* Squad = Memory->CachedSquadManager.Get();

	// Without a squad manager, allow attacking freely
	if (!Squad)
	{
		BBComp->SetValueAsBool(CanAttackKey.SelectedKeyName, true);
		return;
	}

	// Get target from blackboard
	AActor* Target = Cast<AActor>(BBComp->GetValue<UBlackboardKeyType_Object>(TargetActorKey.GetSelectedKeyID()));

	// Attack token management
	const bool bHasToken = Squad->HasAttackToken(Enemy);
	BBComp->SetValueAsBool(CanAttackKey.SelectedKeyName, bHasToken);

	// If we don't have a token, try to get one
	if (!bHasToken)
	{
		if (Squad->RequestAttackToken(Enemy))
		{
			BBComp->SetValueAsBool(CanAttackKey.SelectedKeyName, true);
		}
	}

	// Calculate flank position
	if (Target)
	{
		const FVector FlankPos = Squad->GetFlankPosition(Enemy, Target, FlankDistance);
		BBComp->SetValueAsVector(FlankPositionKey.SelectedKeyName, FlankPos);
	}
}

FString UBTService_SquadCoordination::GetStaticDescription() const
{
	return FString::Printf(TEXT("Squad: tokens→%s, flank→%s (%.0f dist)"),
		*CanAttackKey.SelectedKeyName.ToString(),
		*FlankPositionKey.SelectedKeyName.ToString(),
		FlankDistance);
}

#include "AI/BTService_UpdateCombatState.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "AI/ARPGAIController.h"
#include "Character/ARPGCharacterBase.h"

UBTService_UpdateCombatState::UBTService_UpdateCombatState()
{
	NodeName = TEXT("Update Combat State");

	// Tick at a reasonable interval (not every frame)
	Interval = 0.2f;
	RandomDeviation = 0.05f;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombatState, TargetActorKey), AActor::StaticClass());
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");

	DistanceToTargetKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombatState, DistanceToTargetKey));
	DistanceToTargetKey.SelectedKeyName = TEXT("DistanceToTarget");

	HasLineOfSightKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombatState, HasLineOfSightKey));
	HasLineOfSightKey.SelectedKeyName = TEXT("HasLineOfSight");

	HomeLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateCombatState, HomeLocationKey));
	HomeLocationKey.SelectedKeyName = TEXT("HomeLocation");
}

void UBTService_UpdateCombatState::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
		DistanceToTargetKey.ResolveSelectedKey(*BBAsset);
		HasLineOfSightKey.ResolveSelectedKey(*BBAsset);
		HomeLocationKey.ResolveSelectedKey(*BBAsset);
	}
}

void UBTService_UpdateCombatState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FBTUpdateCombatStateMemory* Memory = reinterpret_cast<FBTUpdateCombatStateMemory*>(NodeMemory);

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!BBComp || !AIC)
	{
		return;
	}

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn)
	{
		return;
	}

	AARPGAIController* ARPGAIC = Cast<AARPGAIController>(AIC);

	// Get current target
	UObject* TargetObj = BBComp->GetValue<UBlackboardKeyType_Object>(TargetActorKey.GetSelectedKeyID());
	AActor* TargetActor = Cast<AActor>(TargetObj);

	if (!TargetActor)
	{
		Memory->NoLOSTimer = 0.f;
		return;
	}

	// --- Check target still alive ---
	const AARPGCharacterBase* TargetChar = Cast<AARPGCharacterBase>(TargetActor);
	if (TargetChar && TargetChar->IsDead())
	{
		if (ARPGAIC)
		{
			ARPGAIC->ClearTarget();
		}
		Memory->NoLOSTimer = 0.f;
		return;
	}

	// --- Update distance ---
	const float Distance = FVector::Dist(Pawn->GetActorLocation(), TargetActor->GetActorLocation());
	BBComp->SetValueAsFloat(DistanceToTargetKey.SelectedKeyName, Distance);

	// --- Leash check ---
	if (ARPGAIC && ARPGAIC->LeashDistance > 0.f)
	{
		const FVector Home = BBComp->GetValueAsVector(HomeLocationKey.SelectedKeyName);
		const float DistFromHome = FVector::Dist(Pawn->GetActorLocation(), Home);
		if (DistFromHome > ARPGAIC->LeashDistance)
		{
			UE_LOG(LogTemp, Log, TEXT("[AI] %s exceeded leash distance (%.0f > %.0f), dropping aggro"),
				*Pawn->GetName(), DistFromHome, ARPGAIC->LeashDistance);
			ARPGAIC->ClearTarget();
			Memory->NoLOSTimer = 0.f;
			return;
		}
	}

	// --- LOS check (simple line trace) ---
	const FVector EyeLocation = Pawn->GetActorLocation() + FVector(0.f, 0.f, 80.f);
	const FVector TargetLocation = TargetActor->GetActorLocation() + FVector(0.f, 0.f, 80.f);

	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(CombatLOS), true, Pawn);
	TraceParams.AddIgnoredActor(TargetActor);

	FHitResult Hit;
	const bool bHasLOS = !Pawn->GetWorld()->LineTraceSingleByChannel(
		Hit, EyeLocation, TargetLocation, ECC_Visibility, TraceParams);

	BBComp->SetValueAsBool(HasLineOfSightKey.SelectedKeyName, bHasLOS);

	// --- Aggro timeout without LOS ---
	if (ARPGAIC && ARPGAIC->AggroTimeoutNoLOS > 0.f)
	{
		if (!bHasLOS)
		{
			Memory->NoLOSTimer += DeltaSeconds;
			if (Memory->NoLOSTimer >= ARPGAIC->AggroTimeoutNoLOS)
			{
				UE_LOG(LogTemp, Log, TEXT("[AI] %s lost LOS for %.1fs, dropping aggro"),
					*Pawn->GetName(), Memory->NoLOSTimer);
				ARPGAIC->ClearTarget();
				Memory->NoLOSTimer = 0.f;
				return;
			}
		}
		else
		{
			Memory->NoLOSTimer = 0.f;
		}
	}
}

uint16 UBTService_UpdateCombatState::GetInstanceMemorySize() const
{
	return sizeof(FBTUpdateCombatStateMemory);
}

FString UBTService_UpdateCombatState::GetStaticDescription() const
{
	return TEXT("Updates DistanceToTarget, HasLineOfSight, validates target alive, checks leash");
}

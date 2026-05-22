#include "AI/ARPGAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Character/ARPGCharacterBase.h"
#include "Character/ARPGEnemyCharacter.h"

const FName AARPGAIController::KEY_TargetActor(TEXT("TargetActor"));
const FName AARPGAIController::KEY_PatrolLocation(TEXT("PatrolLocation"));
const FName AARPGAIController::KEY_DistanceToTarget(TEXT("DistanceToTarget"));
const FName AARPGAIController::KEY_HasLineOfSight(TEXT("HasLineOfSight"));
const FName AARPGAIController::KEY_HomeLocation(TEXT("HomeLocation"));
const FName AARPGAIController::KEY_Archetype(TEXT("Archetype"));
const FName AARPGAIController::KEY_AttackRange(TEXT("AttackRange"));
const FName AARPGAIController::KEY_PreferredDistance(TEXT("PreferredDistance"));
const FName AARPGAIController::KEY_RetreatDistance(TEXT("RetreatDistance"));
const FName AARPGAIController::KEY_AttackCooldown(TEXT("AttackCooldown"));

AARPGAIController::AARPGAIController()
{
	// --- Perception Component ---
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SetPerceptionComponent(*AIPerceptionComp);

	// --- Sight Config ---
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
	SightConfig->SetMaxAge(5.f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = 900.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

	AIPerceptionComp->ConfigureSense(*SightConfig);

	// --- Damage Config ---
	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
	DamageConfig->SetMaxAge(3.f);

	AIPerceptionComp->ConfigureSense(*DamageConfig);

	// --- Hearing Config ---
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = HearingRange;
	HearingConfig->SetMaxAge(3.f);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;

	AIPerceptionComp->ConfigureSense(*HearingConfig);

	// Set sight as the dominant sense
	AIPerceptionComp->SetDominantSense(UAISense_Sight::StaticClass());

	// Bind the per-actor target update callback
	AIPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &AARPGAIController::OnPerceptionUpdated);
}

void AARPGAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Record spawn position as home/leash anchor
	HomeLocation = InPawn ? InPawn->GetActorLocation() : FVector::ZeroVector;

	if (!BehaviorTreeAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("AARPGAIController::OnPossess - No BehaviorTreeAsset set on %s"), *GetName());
		return;
	}

	// UseBlackboard initializes the BlackboardComponent from the BT's blackboard asset
	UBlackboardComponent* BBComp = nullptr;
	if (UseBlackboard(BehaviorTreeAsset->BlackboardAsset, BBComp))
	{
		InitializeBlackboardKeys();
		RunBehaviorTree(BehaviorTreeAsset);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("AARPGAIController::OnPossess - Failed to initialize Blackboard for %s"), *GetName());
	}
}

void AARPGAIController::OnUnPossess()
{
	// Stop the behavior tree when we lose the pawn
	UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent);
	if (BTComp)
	{
		BTComp->StopTree(EBTStopMode::Safe);
	}

	Super::OnUnPossess();
}

void AARPGAIController::InitializeBlackboardKeys()
{
	UBlackboardComponent* BBComp = GetBlackboardComponent();
	if (!BBComp)
	{
		return;
	}

	// Set default values for blackboard keys
	BBComp->SetValueAsObject(KEY_TargetActor, nullptr);
	BBComp->SetValueAsVector(KEY_PatrolLocation, FVector::ZeroVector);
	BBComp->SetValueAsFloat(KEY_DistanceToTarget, 0.f);
	BBComp->SetValueAsBool(KEY_HasLineOfSight, false);
	BBComp->SetValueAsVector(KEY_HomeLocation, HomeLocation);
}

bool AARPGAIController::IsValidTarget(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	// Don't target other enemies
	if (Cast<AARPGEnemyCharacter>(Actor))
	{
		return false;
	}

	// Don't target dead characters
	if (const AARPGCharacterBase* CharBase = Cast<AARPGCharacterBase>(Actor))
	{
		if (CharBase->IsDead())
		{
			return false;
		}
	}

	return true;
}

void AARPGAIController::ClearTarget()
{
	UBlackboardComponent* BBComp = GetBlackboardComponent();
	if (!BBComp)
	{
		return;
	}

	BBComp->SetValueAsObject(KEY_TargetActor, nullptr);
	BBComp->SetValueAsBool(KEY_HasLineOfSight, false);
	BBComp->SetValueAsFloat(KEY_DistanceToTarget, 0.f);

	// Clear focus so the AI stops rotating toward the (now-gone) target
	ClearFocus(EAIFocusPriority::Gameplay);
}

void AARPGAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	UBlackboardComponent* BBComp = GetBlackboardComponent();
	if (!BBComp || !Actor)
	{
		return;
	}

	const bool bIsSight = (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>());
	const bool bIsDamage = (Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>());
	const bool bIsHearing = (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>());

	if (bIsSight)
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			if (!IsValidTarget(Actor))
			{
				return;
			}

			// Gained sight of target — set as current target
			BBComp->SetValueAsObject(KEY_TargetActor, Actor);
			BBComp->SetValueAsBool(KEY_HasLineOfSight, true);

			const float Distance = FVector::Dist(GetPawn()->GetActorLocation(), Actor->GetActorLocation());
			BBComp->SetValueAsFloat(KEY_DistanceToTarget, Distance);

			UE_LOG(LogTemp, Log, TEXT("[Perception] %s spotted %s at %.0f units"),
				*GetName(), *Actor->GetName(), Distance);
		}
		else
		{
			// Lost sight — only clear LOS if this was our current target
			UObject* CurrentTarget = BBComp->GetValueAsObject(KEY_TargetActor);
			if (CurrentTarget == Actor)
			{
				BBComp->SetValueAsBool(KEY_HasLineOfSight, false);

				UE_LOG(LogTemp, Log, TEXT("[Perception] %s lost sight of %s"),
					*GetName(), *Actor->GetName());
			}
		}
	}
	else if (bIsDamage)
	{
		if (!IsValidTarget(Actor))
		{
			return;
		}

		// Damage always aggros — override current target even without LOS
		BBComp->SetValueAsObject(KEY_TargetActor, Actor);

		const float Distance = FVector::Dist(GetPawn()->GetActorLocation(), Actor->GetActorLocation());
		BBComp->SetValueAsFloat(KEY_DistanceToTarget, Distance);

		UE_LOG(LogTemp, Log, TEXT("[Perception] %s took damage from %s (%.0f units away)"),
			*GetName(), *Actor->GetName(), Distance);
	}
	else if (bIsHearing)
	{
		if (!Stimulus.WasSuccessfullySensed())
		{
			return;
		}

		// Only react to hearing if we don't already have a target
		UObject* CurrentTarget = BBComp->GetValueAsObject(KEY_TargetActor);
		if (CurrentTarget != nullptr)
		{
			return;
		}

		if (!IsValidTarget(Actor))
		{
			return;
		}

		// Hearing sets the target but does NOT set LOS
		BBComp->SetValueAsObject(KEY_TargetActor, Actor);

		const float Distance = FVector::Dist(GetPawn()->GetActorLocation(), Actor->GetActorLocation());
		BBComp->SetValueAsFloat(KEY_DistanceToTarget, Distance);

		UE_LOG(LogTemp, Log, TEXT("[Perception] %s heard %s (%.0f units away)"),
			*GetName(), *Actor->GetName(), Distance);
	}
}

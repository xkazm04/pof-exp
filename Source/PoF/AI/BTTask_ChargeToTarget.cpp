#include "AI/BTTask_ChargeToTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/ARPGGameplayTags.h"

UBTTask_ChargeToTarget::UBTTask_ChargeToTarget()
{
	NodeName = TEXT("Charge To Target");
	bNotifyTick = true;
	bNotifyTaskFinished = true;

	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ChargeToTarget, TargetActorKey), AActor::StaticClass());
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");
}

void UBTTask_ChargeToTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}

EBTNodeResult::Type UBTTask_ChargeToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTChargeMemory* Memory = reinterpret_cast<FBTChargeMemory*>(NodeMemory);
	*Memory = FBTChargeMemory(); // Reset

	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	UObject* TargetObj = BBComp ? BBComp->GetValue<UBlackboardKeyType_Object>(TargetActorKey.GetSelectedKeyID()) : nullptr;
	AActor* TargetActor = Cast<AActor>(TargetObj);
	if (!TargetActor)
	{
		return EBTNodeResult::Failed;
	}

	// Boost movement speed
	ACharacter* CharPawn = Cast<ACharacter>(Pawn);
	if (CharPawn && CharPawn->GetCharacterMovement())
	{
		Memory->OriginalSpeed = CharPawn->GetCharacterMovement()->MaxWalkSpeed;
		CharPawn->GetCharacterMovement()->MaxWalkSpeed = Memory->OriginalSpeed * ChargeSpeedMultiplier;
	}

	// Apply State.Charging tag
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			ASC->AddLooseGameplayTag(ARPGGameplayTags::State_Charging);
		}
	}

	// Issue move-to request
	FAIMoveRequest MoveReq;
	MoveReq.SetGoalActor(TargetActor);
	MoveReq.SetAcceptanceRadius(AcceptanceRadius);

	AIC->MoveTo(MoveReq);

	return EBTNodeResult::InProgress;
}

void UBTTask_ChargeToTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTChargeMemory* Memory = reinterpret_cast<FBTChargeMemory*>(NodeMemory);

	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!Pawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// Vulnerability window phase
	if (Memory->bInVulnerabilityWindow)
	{
		Memory->VulnerabilityElapsed += DeltaSeconds;
		if (Memory->VulnerabilityElapsed >= VulnerabilityDuration)
		{
			// Remove vulnerable tag
			if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn))
			{
				if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
				{
					ASC->RemoveLooseGameplayTag(ARPGGameplayTags::State_Vulnerable);
				}
			}
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		return;
	}

	// Charging phase
	Memory->ElapsedTime += DeltaSeconds;

	UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();
	UObject* TargetObj = BBComp ? BBComp->GetValue<UBlackboardKeyType_Object>(TargetActorKey.GetSelectedKeyID()) : nullptr;
	AActor* TargetActor = Cast<AActor>(TargetObj);
	if (!TargetActor)
	{
		RestoreSpeed(Pawn, Memory->OriginalSpeed);
		CleanupTags(Pawn);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const float Distance = FVector::Dist(Pawn->GetActorLocation(), TargetActor->GetActorLocation());

	// Check if we've reached the target
	if (Distance <= AcceptanceRadius)
	{
		AIC->StopMovement();
		StartVulnerabilityWindow(OwnerComp, Memory);
		return;
	}

	// Safety timeout
	if (Memory->ElapsedTime >= MaxChargeDuration)
	{
		AIC->StopMovement();
		StartVulnerabilityWindow(OwnerComp, Memory);
		return;
	}
}

EBTNodeResult::Type UBTTask_ChargeToTarget::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTChargeMemory* Memory = reinterpret_cast<FBTChargeMemory*>(NodeMemory);

	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;

	if (AIC)
	{
		AIC->StopMovement();
	}

	if (Pawn)
	{
		RestoreSpeed(Pawn, Memory->OriginalSpeed);
		CleanupTags(Pawn);
	}

	return EBTNodeResult::Aborted;
}

void UBTTask_ChargeToTarget::StartVulnerabilityWindow(UBehaviorTreeComponent& OwnerComp, FBTChargeMemory* Memory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
	if (!Pawn) return;

	// Restore speed
	RestoreSpeed(Pawn, Memory->OriginalSpeed);

	// Swap tags: remove Charging, add Vulnerable
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			ASC->RemoveLooseGameplayTag(ARPGGameplayTags::State_Charging);
			ASC->AddLooseGameplayTag(ARPGGameplayTags::State_Vulnerable);
		}
	}

	Memory->bInVulnerabilityWindow = true;
	Memory->VulnerabilityElapsed = 0.f;
}

void UBTTask_ChargeToTarget::RestoreSpeed(APawn* Pawn, float OriginalSpeed)
{
	ACharacter* CharPawn = Cast<ACharacter>(Pawn);
	if (CharPawn && CharPawn->GetCharacterMovement() && OriginalSpeed > 0.f)
	{
		CharPawn->GetCharacterMovement()->MaxWalkSpeed = OriginalSpeed;
	}
}

void UBTTask_ChargeToTarget::CleanupTags(APawn* Pawn)
{
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			ASC->RemoveLooseGameplayTag(ARPGGameplayTags::State_Charging);
			ASC->RemoveLooseGameplayTag(ARPGGameplayTags::State_Vulnerable);
		}
	}
}

FString UBTTask_ChargeToTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("Charge at %.1fx speed (max %.1fs), then vulnerable for %.1fs"),
		ChargeSpeedMultiplier, MaxChargeDuration, VulnerabilityDuration);
}

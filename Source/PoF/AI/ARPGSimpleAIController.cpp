#include "AI/ARPGSimpleAIController.h"
#include "Character/ARPGEnemyCharacter.h"
#include "Character/ARPGCharacterBase.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"

AARPGSimpleAIController::AARPGSimpleAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	AttackAbilityTag = ARPGGameplayTags::Ability_Enemy_Melee;
}

void AARPGSimpleAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ControlledEnemy = Cast<AARPGEnemyCharacter>(InPawn);
	// Large initial value so the first in-range tick attacks immediately.
	TimeSinceLastAttack = 1000.f;
}

void AARPGSimpleAIController::OnUnPossess()
{
	ControlledEnemy = nullptr;
	Super::OnUnPossess();
}

void AARPGSimpleAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!ControlledEnemy.IsValid() || ControlledEnemy->IsDead())
	{
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!PlayerPawn)
	{
		return;
	}
	if (const AARPGCharacterBase* PlayerChar = Cast<AARPGCharacterBase>(PlayerPawn))
	{
		if (PlayerChar->IsDead())
		{
			return;
		}
	}

	TimeSinceLastAttack += DeltaSeconds;

	const FVector EnemyLoc = ControlledEnemy->GetActorLocation();
	const FVector PlayerLoc = PlayerPawn->GetActorLocation();
	const float Dist = FVector::Dist(EnemyLoc, PlayerLoc);
	const float Range = ControlledEnemy->GetAttackRange();

	if (Dist > Range)
	{
		// Chase: steer straight toward the player (nav-independent on the flat arena).
		const FVector Dir = (PlayerLoc - EnemyLoc).GetSafeNormal2D();
		ControlledEnemy->AddMovementInput(Dir, 1.f);
		return;
	}

	// In range: face the player so the front-arc damage lands, then attack on cooldown.
	const FVector ToPlayer = PlayerLoc - EnemyLoc;
	ControlledEnemy->SetActorRotation(FRotator(0.f, ToPlayer.Rotation().Yaw, 0.f));

	if (TimeSinceLastAttack >= ControlledEnemy->GetAttackCooldown())
	{
		if (UAbilitySystemComponent* ASC = ControlledEnemy->GetAbilitySystemComponent())
		{
			FGameplayTagContainer AttackTags;
			AttackTags.AddTag(AttackAbilityTag);
			if (ASC->TryActivateAbilitiesByTag(AttackTags))
			{
				TimeSinceLastAttack = 0.f;
			}
		}
	}
}

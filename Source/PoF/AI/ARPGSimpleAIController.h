#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "ARPGSimpleAIController.generated.h"

class AARPGEnemyCharacter;

/**
 * Minimal pure-C++ enemy AI controller — no Behaviour Tree, no blackboard.
 *
 * Each tick it finds the player, steers straight toward them with
 * AddMovementInput (nav-independent — works on the bare arena floor), and once
 * within the enemy's AttackRange faces the player and activates the enemy's
 * melee ability by tag, respecting AttackCooldown.
 *
 * Set as AIControllerClass on BP_VSEnemy to make the slice enemy hostile
 * without authoring a Behaviour Tree asset (the binary-content wall).
 */
UCLASS()
class POF_API AARPGSimpleAIController : public AAIController
{
	GENERATED_BODY()

public:
	AARPGSimpleAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	/** Tag used to activate the controlled enemy's attack ability (matches GA_EnemyMeleeAttack's asset tag). */
	UPROPERTY(EditDefaultsOnly, Category = "Simple AI")
	FGameplayTag AttackAbilityTag;

private:
	TWeakObjectPtr<AARPGEnemyCharacter> ControlledEnemy;
	float TimeSinceLastAttack = 0.f;
};

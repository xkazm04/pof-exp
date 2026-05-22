#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ARPGAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Damage;
class UAISenseConfig_Hearing;
class UBehaviorTree;
class UBlackboardComponent;

UCLASS()
class POF_API AARPGAIController : public AAIController
{
	GENERATED_BODY()

public:
	AARPGAIController();

	// Behavior Tree asset to run when possessing a pawn
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	// Blackboard key names — core
	static const FName KEY_TargetActor;
	static const FName KEY_PatrolLocation;
	static const FName KEY_DistanceToTarget;
	static const FName KEY_HasLineOfSight;
	static const FName KEY_HomeLocation;

	// Blackboard key names — archetype (written by enemy character on possess)
	static const FName KEY_Archetype;
	static const FName KEY_AttackRange;
	static const FName KEY_PreferredDistance;
	static const FName KEY_RetreatDistance;
	static const FName KEY_AttackCooldown;

	// =====================================================================
	// Perception
	// =====================================================================

	UFUNCTION(BlueprintPure, Category = "AI|Perception")
	UAIPerceptionComponent* GetAIPerception() const { return AIPerceptionComp; }

	// =====================================================================
	// Aggro / Leash
	// =====================================================================

	/**
	 * Clear the current target and return to passive state.
	 * Called when target dies, when leash distance is exceeded, etc.
	 */
	UFUNCTION(BlueprintCallable, Category = "AI|Aggro")
	void ClearTarget();

	/** Maximum distance from home before the AI drops aggro and returns. 0 = no leash. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Aggro")
	float LeashDistance = 3000.f;

	/** Time in seconds without LOS before the AI drops aggro. 0 = never. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Aggro")
	float AggroTimeoutNoLOS = 8.f;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// --- Perception tuning (exposed so BPs can override per-enemy) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Sight")
	float SightRadius = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Sight")
	float LoseSightRadius = 2000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Sight")
	float PeripheralVisionAngleDegrees = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Hearing")
	float HearingRange = 1200.f;

private:
	UPROPERTY(VisibleAnywhere, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComp;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	/** Spawn location — used as leash anchor and patrol center. */
	FVector HomeLocation = FVector::ZeroVector;

	void InitializeBlackboardKeys();

	/** Returns true if the actor is a valid aggro target (alive player, not another enemy). */
	bool IsValidTarget(AActor* Actor) const;

	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
};

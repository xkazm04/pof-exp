#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/ARPGStateGraphRules.h"
#include "ARPGStateTreeAIComponent.generated.h"

class UStateTreeComponent;

/**
 * Enemy-AI runtime component (catalog pipeline: state-graph → UE Packaging). Owns the
 * engine's UStateTreeComponent (the behaviour-graph runner) and drives the six-state FSM
 * (Idle→Patrol→Chase→Attack→{Flee,Dead}) using the pure edge guards in UARPGStateGraphRules,
 * so the runtime and the L3 gate (VSStateGraphTest) share one source of truth. The StateTree
 * asset keys its transitions off the same guards; this component is the C++ that evaluates
 * them from the blackboard each tick and exposes the current State.AI.<x> tag.
 *
 * The browser duel realization (saber-arpg src/data/stategraph.js + ai/brain.js) runs this
 * same graph off the same artifact — dual execution, one authored source.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class POF_API UARPGStateTreeAIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGStateTreeAIComponent();

	/** The engine StateTree runner that executes this enemy's behaviour graph. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeComponent> StateTree;

	/** Current FSM state (mirrors the State.AI.<State> gameplay tag the StateTree drives). */
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	EARPGAIState CurrentState = EARPGAIState::Idle;

	/** bFleeUsed — FLEE is authored as once per engagement; cleared by ResetEngagement(). */
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	bool bFleeUsed = false;

	/** Remaining FLEE window (s). FleeTimerExpired == (FleeTimer <= 0). */
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	float FleeTimer = 0.f;

	/** Accrues while IDLE with no target; gates IDLE→PATROL at IdleDwellSeconds. */
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	float IdleDwellTimer = 0.f;

	/** LeashDistanceExceeded — set by the spawner when the enemy is dragged off post. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bLeashExceeded = false;

	/** WaypointQueueEmpty — set by the patrol component when a lap completes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	bool bWaypointQueueEmpty = true;

	/** Tick the FSM's own timers (flee window, idle dwell) before EvaluateNext. */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void AdvanceTimers(float DeltaSeconds, bool bHasTarget);

	/** Clear the per-engagement latches (new encounter / respawn). */
	UFUNCTION(BlueprintCallable, Category = "AI")
	void ResetEngagement();

	/** Evaluate + apply the next FSM state from blackboard reads, using the shared guards.
	 *  Returns the resulting state; DEAD is terminal (no further transitions).
	 *  AlertCooldown carries the IdleDwellTimer value the IDLE→PATROL guard reads. */
	UFUNCTION(BlueprintCallable, Category = "AI")
	EARPGAIState EvaluateNext(bool bHasTarget, bool bTargetInAlertRadius, float AlertCooldown,
		float DistanceToTargetCm, float HealthPct);
};

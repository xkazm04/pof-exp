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

	/** Evaluate + apply the next FSM state from blackboard reads, using the shared guards.
	 *  Returns the resulting state; DEAD is terminal (no further transitions). */
	UFUNCTION(BlueprintCallable, Category = "AI")
	EARPGAIState EvaluateNext(bool bHasTarget, bool bTargetInAlertRadius, float AlertCooldown,
		float DistanceToTargetCm, float HealthPct);
};

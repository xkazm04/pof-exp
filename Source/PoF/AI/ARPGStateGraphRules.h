#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGStateGraphRules.generated.h"

/**
 * Generic enemy-AI finite state machine rules (catalog pipeline: state-graph).
 * Six states Idle→Patrol→Chase→Attack→{Flee,Dead}; Dead is the only terminal.
 * Encodes the documented edge guards as pure predicates so the game FSM
 * (UStateTreeComponent) and the L3 gate (VSStateGraphTest) share one source of
 * truth. Mirrors src/lib/catalog/pipelines/state-graph.ts.
 */
UENUM(BlueprintType)
enum class EARPGAIState : uint8
{
	Idle,
	Patrol,
	Chase,
	Attack,
	Flee,
	Dead
};

UCLASS()
class POF_API UARPGStateGraphRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr int32 NumStates = 6;
	static constexpr float FleeHealthPct = 0.20f;
	static constexpr float AttackRangeCm = 150.f;
	static constexpr float AlertCooldownSeconds = 5.f;

	/** Idle→Patrol: no target and the alert cooldown has elapsed (> 5 s). */
	UFUNCTION(BlueprintPure, Category = "AI|StateGraph")
	static bool ShouldIdleToPatrol(bool bHasTarget, float AlertCooldown)
	{ return !bHasTarget && AlertCooldown > AlertCooldownSeconds; }

	/** Idle/Patrol→Chase: a target exists and is within the alert radius. */
	UFUNCTION(BlueprintPure, Category = "AI|StateGraph")
	static bool ShouldEnterChase(bool bHasTarget, bool bTargetInAlertRadius)
	{ return bHasTarget && bTargetInAlertRadius; }

	/** Chase→Attack: within melee range (≤ 150 cm). */
	UFUNCTION(BlueprintPure, Category = "AI|StateGraph")
	static bool ShouldChaseToAttack(float DistanceToTargetCm)
	{ return DistanceToTargetCm <= AttackRangeCm; }

	/** Chase→Flee: health dropped below the flee threshold (< 20%). */
	UFUNCTION(BlueprintPure, Category = "AI|StateGraph")
	static bool ShouldChaseToFlee(float HealthPct)
	{ return HealthPct < FleeHealthPct; }

	/** Dead is the only terminal state (no outgoing edges; not saved). */
	UFUNCTION(BlueprintPure, Category = "AI|StateGraph")
	static bool IsTerminal(EARPGAIState State) { return State == EARPGAIState::Dead; }
};

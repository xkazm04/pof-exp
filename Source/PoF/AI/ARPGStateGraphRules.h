#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGStateGraphRules.generated.h"

/**
 * Generic enemy-AI finite state machine rules (catalog pipeline: state-graph).
 * Six states Idle→Patrol→Chase→Attack→{Flee,Dead}; Dead is the only terminal
 * (Flee is a bounded recovery state that always exits). Encodes the documented
 * edge guards as pure predicates so the game FSM (UStateTreeComponent), the
 * browser duel realization and the L3 gate (VSStateGraphTest) share one source
 * of truth.
 *
 * The constants below MIRROR the authored artifact (state-graph :: anim-atk-combo1,
 * steps "State Graph" + "Transition Rules"). Dual-execution contract: the browser
 * preview parses these same numbers straight out of that artifact, so any drift
 * here shows up as the two realizations disagreeing. Do not retune in code —
 * retune the artifact, then mirror it here.
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

	/** HealthPct < 0.25 opens FLEE — once per engagement (bFleeUsed latch). */
	static constexpr float FleeHealthPct = 0.25f;
	/** CHASE→ATTACK enter band (cm). */
	static constexpr float AttackEnterCm = 175.f;
	/** ATTACK→CHASE exit band (cm) — wider than the enter band, so the edge has
	 *  hysteresis and a target hovering at the boundary cannot flip the FSM every tick. */
	static constexpr float AttackExitCm = 250.f;
	/** Perception radius that sets TargetInAlertRadius (cm). */
	static constexpr float AlertRadiusCm = 800.f;
	/** IdleDwellTimer needed before IDLE→PATROL (s). */
	static constexpr float IdleDwellSeconds = 4.f;
	/** NoLOSDuration before a chased target is dropped (s). */
	static constexpr float NoLosSeconds = 4.f;
	/** Bounded FLEE window (s) — the timer never kills; only a lethal hit does. */
	static constexpr float FleeDurationSeconds = 6.f;

	/** Per-state locomotion speeds (cm/s) as authored on the graph nodes. */
	static constexpr float PatrolSpeedCms = 250.f;
	static constexpr float ChaseSpeedCms = 500.f;
	static constexpr float FleeSpeedCms = 480.f;

	/** Idle→Patrol: no target and the idle dwell timer has elapsed (≥ 4 s). */
	UFUNCTION(BlueprintPure, Category = "AI|StateGraph")
	static bool ShouldIdleToPatrol(bool bHasTarget, float IdleDwellTimer)
	{ return !bHasTarget && IdleDwellTimer >= IdleDwellSeconds; }

	/** Idle/Patrol→Chase: a target exists and is within the alert radius. */
	UFUNCTION(BlueprintPure, Category = "AI|StateGraph")
	static bool ShouldEnterChase(bool bHasTarget, bool bTargetInAlertRadius)
	{ return bHasTarget && bTargetInAlertRadius; }

	/** Perception: is the target inside the alert radius (drives TargetInAlertRadius)? */
	UFUNCTION(BlueprintPure, Category = "AI|StateGraph")
	static bool IsTargetPerceived(float DistanceToTargetCm)
	{ return DistanceToTargetCm <= AlertRadiusCm; }

	/** Chase→Attack: inside the enter band (≤ 175 cm). */
	UFUNCTION(BlueprintPure, Category = "AI|StateGraph")
	static bool ShouldChaseToAttack(float DistanceToTargetCm)
	{ return DistanceToTargetCm <= AttackEnterCm; }

	/** Attack→Chase: past the wider exit band (> 250 cm). */
	UFUNCTION(BlueprintPure, Category = "AI|StateGraph")
	static bool ShouldAttackToChase(float DistanceToTargetCm)
	{ return DistanceToTargetCm > AttackExitCm; }

	/** →Flee: below the flee threshold AND the once-per-engagement latch is free. */
	UFUNCTION(BlueprintPure, Category = "AI|StateGraph")
	static bool ShouldEnterFlee(float HealthPct, bool bFleeUsed)
	{ return HealthPct < FleeHealthPct && !bFleeUsed; }

	/** Flee→Chase ("cornered — turn and fight"): window expired, target still held. */
	UFUNCTION(BlueprintPure, Category = "AI|StateGraph")
	static bool ShouldFleeToChase(bool bFleeTimerExpired, bool bHasTarget, bool bLeashExceeded)
	{ return bFleeTimerExpired && bHasTarget && !bLeashExceeded; }

	/** Flee→Idle ("escaped"): window expired with no target, or leashed back to post. */
	UFUNCTION(BlueprintPure, Category = "AI|StateGraph")
	static bool ShouldFleeToIdle(bool bFleeTimerExpired, bool bHasTarget, bool bLeashExceeded)
	{ return bFleeTimerExpired && (!bHasTarget || bLeashExceeded); }

	/** Dead is the only terminal state (no outgoing edges; not saved). */
	UFUNCTION(BlueprintPure, Category = "AI|StateGraph")
	static bool IsTerminal(EARPGAIState State) { return State == EARPGAIState::Dead; }
};

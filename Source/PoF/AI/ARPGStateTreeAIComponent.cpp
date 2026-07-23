#include "AI/ARPGStateTreeAIComponent.h"
#include "Components/StateTreeComponent.h"

UARPGStateTreeAIComponent::UARPGStateTreeAIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	StateTree = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTree"));
}

void UARPGStateTreeAIComponent::AdvanceTimers(float DeltaSeconds, bool bHasTarget)
{
	if (CurrentState == EARPGAIState::Flee)
	{
		FleeTimer = FMath::Max(0.f, FleeTimer - DeltaSeconds);
	}
	// IdleDwellTimer only accrues while genuinely targetless (the IDLE→PATROL guard).
	IdleDwellTimer = (CurrentState == EARPGAIState::Idle && !bHasTarget)
		? IdleDwellTimer + DeltaSeconds
		: 0.f;
}

void UARPGStateTreeAIComponent::ResetEngagement()
{
	bFleeUsed = false;
	FleeTimer = 0.f;
	IdleDwellTimer = 0.f;
}

EARPGAIState UARPGStateTreeAIComponent::EvaluateNext(bool bHasTarget, bool bTargetInAlertRadius,
	float AlertCooldown, float DistanceToTargetCm, float HealthPct)
{
	using R = UARPGStateGraphRules;
	using S = EARPGAIState;

	// Dead is terminal — never transitions out.
	if (R::IsTerminal(CurrentState))
	{
		return CurrentState;
	}

	// HealthPct ≤ 0 → DEAD from any live state (authored on every node).
	if (HealthPct <= 0.f)
	{
		CurrentState = S::Dead;
		return CurrentState;
	}

	// →FLEE is the critical guard: evaluated before the per-state edges and latched
	// by bFleeUsed so it can fire at most once per engagement.
	if ((CurrentState == S::Chase || CurrentState == S::Attack) &&
		R::ShouldEnterFlee(HealthPct, bFleeUsed))
	{
		bFleeUsed = true;
		FleeTimer = R::FleeDurationSeconds;
		CurrentState = S::Flee;
		return CurrentState;
	}

	switch (CurrentState)
	{
	case S::Idle:
		if (R::ShouldEnterChase(bHasTarget, bTargetInAlertRadius)) CurrentState = S::Chase;
		else if (R::ShouldIdleToPatrol(bHasTarget, AlertCooldown)) CurrentState = S::Patrol;
		break;
	case S::Patrol:
		if (R::ShouldEnterChase(bHasTarget, bTargetInAlertRadius)) CurrentState = S::Chase;
		else if (!bHasTarget && bWaypointQueueEmpty) CurrentState = S::Idle;
		break;
	case S::Chase:
		if (R::ShouldChaseToAttack(DistanceToTargetCm)) CurrentState = S::Attack;
		else if (!bHasTarget) CurrentState = S::Idle;
		break;
	case S::Attack:
		// Wider exit than enter — the authored hysteresis band.
		if (R::ShouldAttackToChase(DistanceToTargetCm)) CurrentState = S::Chase;
		else if (!bHasTarget) CurrentState = S::Idle;
		break;
	case S::Flee:
	{
		// The flee timer never kills — it just ends the window; a lethal hit is the
		// only death out of FLEE (handled by the HealthPct ≤ 0 check above).
		const bool bExpired = FleeTimer <= 0.f;
		if (R::ShouldFleeToChase(bExpired, bHasTarget, bLeashExceeded)) CurrentState = S::Chase;
		else if (R::ShouldFleeToIdle(bExpired, bHasTarget, bLeashExceeded)) CurrentState = S::Idle;
		break;
	}
	default:
		break;
	}

	return CurrentState;
}

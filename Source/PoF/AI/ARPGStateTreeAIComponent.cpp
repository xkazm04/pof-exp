#include "AI/ARPGStateTreeAIComponent.h"
#include "Components/StateTreeComponent.h"

UARPGStateTreeAIComponent::UARPGStateTreeAIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	StateTree = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTree"));
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

	switch (CurrentState)
	{
	case S::Idle:
		if (R::ShouldEnterChase(bHasTarget, bTargetInAlertRadius)) CurrentState = S::Chase;
		else if (R::ShouldIdleToPatrol(bHasTarget, AlertCooldown)) CurrentState = S::Patrol;
		break;
	case S::Patrol:
		if (R::ShouldEnterChase(bHasTarget, bTargetInAlertRadius)) CurrentState = S::Chase;
		else if (!bHasTarget) CurrentState = S::Idle;
		break;
	case S::Chase:
		if (R::ShouldChaseToFlee(HealthPct)) CurrentState = S::Flee;
		else if (R::ShouldChaseToAttack(DistanceToTargetCm)) CurrentState = S::Attack;
		else if (!bHasTarget) CurrentState = S::Idle;
		break;
	case S::Attack:
		if (R::ShouldChaseToFlee(HealthPct)) CurrentState = S::Flee;
		else if (DistanceToTargetCm > R::AttackRangeCm) CurrentState = S::Chase;
		break;
	case S::Flee:
		// Recover to Idle once health is back above the flee threshold.
		if (HealthPct >= R::FleeHealthPct) CurrentState = S::Idle;
		break;
	default:
		break;
	}

	return CurrentState;
}

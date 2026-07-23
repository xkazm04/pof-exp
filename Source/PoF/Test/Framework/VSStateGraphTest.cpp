#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/ARPGStateGraphRules.h"
#include "AI/ARPGStateTreeAIComponent.h"

/**
 * State-graph L3 gate (state-graph, runtimeDeferred('VSStateGraphTest')). Asserts the
 * production FSM edge guards + no-deadlock/reachability invariants. Pure logic — headless.
 *
 * Every constant asserted here is the number authored in the catalog artifact
 * (state-graph :: anim-atk-combo1). The browser duel realization parses those same
 * numbers out of the artifact at runtime, so this gate is what keeps the two
 * executions honest: retune the artifact and both sides move, or this goes red.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSStateGraphTest,
	"Project.Functional Tests.PoF.StateGraph.VSStateGraphTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSStateGraphTest::RunTest(const FString& /*Parameters*/)
{
	using S = EARPGAIState;
	using R = UARPGStateGraphRules;

	TestEqual(TEXT("FSM has 6 states"), R::NumStates, 6);

	// --- the authored constants (artifact <-> code mirror) ---
	TestEqual(TEXT("flee threshold is the authored 25%"), R::FleeHealthPct, 0.25f);
	TestEqual(TEXT("attack enter band is the authored 175 cm"), R::AttackEnterCm, 175.f);
	TestEqual(TEXT("attack exit band is the authored 250 cm"), R::AttackExitCm, 250.f);
	TestEqual(TEXT("alert radius is the authored 800 cm"), R::AlertRadiusCm, 800.f);
	TestEqual(TEXT("idle dwell is the authored 4 s"), R::IdleDwellSeconds, 4.f);
	TestEqual(TEXT("no-LOS drop is the authored 4 s"), R::NoLosSeconds, 4.f);
	TestEqual(TEXT("flee window is the authored 6 s"), R::FleeDurationSeconds, 6.f);
	TestEqual(TEXT("patrol speed is the authored 250 cm/s"), R::PatrolSpeedCms, 250.f);
	TestEqual(TEXT("chase speed is the authored 500 cm/s"), R::ChaseSpeedCms, 500.f);
	TestEqual(TEXT("flee speed is the authored 480 cm/s"), R::FleeSpeedCms, 480.f);

	// Idle->Patrol guard: no target AND the idle dwell timer elapsed (>= 4 s).
	TestTrue(TEXT("Idle->Patrol at the 4 s dwell"), R::ShouldIdleToPatrol(false, 4.f));
	TestFalse(TEXT("no Idle->Patrol before the dwell"), R::ShouldIdleToPatrol(false, 3.9f));
	TestFalse(TEXT("no Idle->Patrol while a target exists"), R::ShouldIdleToPatrol(true, 6.f));

	// Perception -> TargetInAlertRadius at 800 cm.
	TestTrue(TEXT("perceived at 800 cm"), R::IsTargetPerceived(800.f));
	TestFalse(TEXT("not perceived at 801 cm"), R::IsTargetPerceived(801.f));

	// Enter Chase on perception (target in alert radius).
	TestTrue(TEXT("->Chase on sight (target in radius)"), R::ShouldEnterChase(true, true));
	TestFalse(TEXT("no ->Chase without a target"), R::ShouldEnterChase(false, true));

	// Attack band hysteresis: enter <=175, exit >250 — the gap must NOT flip either way.
	TestTrue(TEXT("Chase->Attack at 175 cm"), R::ShouldChaseToAttack(175.f));
	TestFalse(TEXT("no Chase->Attack at 176 cm"), R::ShouldChaseToAttack(176.f));
	TestFalse(TEXT("no Attack->Chase at 250 cm"), R::ShouldAttackToChase(250.f));
	TestTrue(TEXT("Attack->Chase at 251 cm"), R::ShouldAttackToChase(251.f));
	TestFalse(TEXT("200 cm holds ATTACK (inside the hysteresis gap)"), R::ShouldAttackToChase(200.f));
	TestFalse(TEXT("200 cm does not re-enter from CHASE"), R::ShouldChaseToAttack(200.f));

	// ->Flee below 25%, once per engagement (bFleeUsed latch).
	TestTrue(TEXT("->Flee at 24% health"), R::ShouldEnterFlee(0.24f, false));
	TestFalse(TEXT("no ->Flee at 25% health"), R::ShouldEnterFlee(0.25f, false));
	TestFalse(TEXT("no second ->Flee once the latch is set"), R::ShouldEnterFlee(0.10f, true));

	// Flee exits: cornered -> Chase, escaped/leashed -> Idle. The timer never kills.
	TestTrue(TEXT("Flee->Chase when cornered"), R::ShouldFleeToChase(true, true, false));
	TestFalse(TEXT("no Flee->Chase before the window expires"), R::ShouldFleeToChase(false, true, false));
	TestTrue(TEXT("Flee->Idle when escaped"), R::ShouldFleeToIdle(true, false, false));
	TestTrue(TEXT("Flee->Idle when leashed"), R::ShouldFleeToIdle(true, true, true));

	// Dead is the ONLY terminal — no deadlock among the live states.
	TestTrue(TEXT("Dead is terminal"), R::IsTerminal(S::Dead));
	TestFalse(TEXT("Idle is not terminal"), R::IsTerminal(S::Idle));
	TestFalse(TEXT("Patrol is not terminal"), R::IsTerminal(S::Patrol));
	TestFalse(TEXT("Chase is not terminal"), R::IsTerminal(S::Chase));
	TestFalse(TEXT("Attack is not terminal"), R::IsTerminal(S::Attack));
	TestFalse(TEXT("Flee is not terminal (bounded recovery, always exits)"), R::IsTerminal(S::Flee));

	// --- the runtime component walks the same graph -------------------------
	UARPGStateTreeAIComponent* AI = NewObject<UARPGStateTreeAIComponent>(GetTransientPackage());
	if (!TestNotNull(TEXT("state-tree AI component instantiates"), AI))
	{
		return false;
	}
	AI->bWaypointQueueEmpty = true;

	// IDLE -> (dwell) -> PATROL with no target.
	TestEqual(TEXT("starts IDLE"), AI->CurrentState, S::Idle);
	AI->AdvanceTimers(4.f, /*bHasTarget*/ false);
	TestEqual(TEXT("IDLE->PATROL after the dwell"),
		AI->EvaluateNext(false, false, AI->IdleDwellTimer, 900.f, 1.f), S::Patrol);

	// PATROL -> CHASE on perception, then CHASE -> ATTACK inside the enter band.
	TestEqual(TEXT("PATROL->CHASE on sight"),
		AI->EvaluateNext(true, true, 0.f, 700.f, 1.f), S::Chase);
	TestEqual(TEXT("CHASE->ATTACK at 150 cm"),
		AI->EvaluateNext(true, true, 0.f, 150.f, 1.f), S::Attack);

	// Hysteresis: 200 cm is inside the gap — ATTACK must hold.
	TestEqual(TEXT("ATTACK holds at 200 cm (hysteresis gap)"),
		AI->EvaluateNext(true, true, 0.f, 200.f, 1.f), S::Attack);
	TestEqual(TEXT("ATTACK->CHASE past 250 cm"),
		AI->EvaluateNext(true, true, 0.f, 260.f, 1.f), S::Chase);

	// ->FLEE under 25%, bounded, then cornered -> CHASE, and never a second time.
	TestEqual(TEXT("CHASE->FLEE under 25%"),
		AI->EvaluateNext(true, true, 0.f, 300.f, 0.24f), S::Flee);
	TestTrue(TEXT("flee latch set"), AI->bFleeUsed);
	TestEqual(TEXT("FLEE holds while the window runs"),
		AI->EvaluateNext(true, true, 0.f, 300.f, 0.24f), S::Flee);
	AI->AdvanceTimers(R::FleeDurationSeconds, /*bHasTarget*/ true);
	TestEqual(TEXT("FLEE->CHASE cornered when the window expires"),
		AI->EvaluateNext(true, true, 0.f, 300.f, 0.24f), S::Chase);
	TestEqual(TEXT("still under 25% but the latch blocks a second FLEE"),
		AI->EvaluateNext(true, true, 0.f, 300.f, 0.10f), S::Chase);

	// Lethal hit -> DEAD, and DEAD is absorbing.
	TestEqual(TEXT("->DEAD at 0 health"),
		AI->EvaluateNext(true, true, 0.f, 100.f, 0.f), S::Dead);
	TestEqual(TEXT("DEAD absorbs"),
		AI->EvaluateNext(true, true, 0.f, 100.f, 1.f), S::Dead);

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

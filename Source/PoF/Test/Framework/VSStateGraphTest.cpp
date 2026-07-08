#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/ARPGStateGraphRules.h"

/**
 * State-graph L3 gate (state-graph, runtimeDeferred('VSStateGraphTest')). Asserts the
 * production FSM edge guards + no-deadlock/reachability invariants. Pure logic — headless.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSStateGraphTest,
	"Project.Functional Tests.PoF.StateGraph.VSStateGraphTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSStateGraphTest::RunTest(const FString& /*Parameters*/)
{
	using S = EARPGAIState;

	TestEqual(TEXT("FSM has 6 states"), UARPGStateGraphRules::NumStates, 6);

	// Idle→Patrol guard: no target AND cooldown elapsed.
	TestTrue(TEXT("Idle→Patrol when no target & cooldown > 5s"),
		UARPGStateGraphRules::ShouldIdleToPatrol(false, 6.f));
	TestFalse(TEXT("no Idle→Patrol while cooldown ≤ 5s"),
		UARPGStateGraphRules::ShouldIdleToPatrol(false, 4.f));
	TestFalse(TEXT("no Idle→Patrol while a target exists"),
		UARPGStateGraphRules::ShouldIdleToPatrol(true, 6.f));

	// Enter Chase on perception (target in alert radius).
	TestTrue(TEXT("→Chase on sight (target in radius)"),
		UARPGStateGraphRules::ShouldEnterChase(true, true));
	TestFalse(TEXT("no →Chase without a target"),
		UARPGStateGraphRules::ShouldEnterChase(false, true));

	// Chase→Attack within melee range (≤150cm).
	TestTrue(TEXT("Chase→Attack at 150cm"), UARPGStateGraphRules::ShouldChaseToAttack(150.f));
	TestFalse(TEXT("no Chase→Attack at 151cm"), UARPGStateGraphRules::ShouldChaseToAttack(151.f));

	// Chase→Flee below the flee health threshold (<20%).
	TestTrue(TEXT("Chase→Flee at 19% health"), UARPGStateGraphRules::ShouldChaseToFlee(0.19f));
	TestFalse(TEXT("no Chase→Flee at 20% health"), UARPGStateGraphRules::ShouldChaseToFlee(0.20f));

	// Dead is the ONLY terminal — no deadlock among the live states.
	TestTrue(TEXT("Dead is terminal"), UARPGStateGraphRules::IsTerminal(S::Dead));
	TestFalse(TEXT("Idle is not terminal"), UARPGStateGraphRules::IsTerminal(S::Idle));
	TestFalse(TEXT("Patrol is not terminal"), UARPGStateGraphRules::IsTerminal(S::Patrol));
	TestFalse(TEXT("Chase is not terminal"), UARPGStateGraphRules::IsTerminal(S::Chase));
	TestFalse(TEXT("Attack is not terminal"), UARPGStateGraphRules::IsTerminal(S::Attack));
	TestFalse(TEXT("Flee is not terminal"), UARPGStateGraphRules::IsTerminal(S::Flee));

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

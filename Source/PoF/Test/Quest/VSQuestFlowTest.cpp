#include "Test/Quest/VSQuestFlowTest.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Quest/ARPGQuestSubsystem.h"
#include "Quest/ARPGQuestDefinition.h"
#include "Quest/ARPGQuestTypes.h"
#include "Engine/GameInstance.h"

/**
 * Quest Test Gate — stage/objective flow through UARPGQuestSubsystem.
 *
 * Pure logic gate (no PIE/world/map needed — safe under shared-tree
 * concurrency). Drives the subsystem through a genuine quest chain:
 * prerequisite gating, accept, event-driven objective progress with clamping,
 * optional objectives ignored by auto-complete, manual turn-in, fail path,
 * abandon rules, and terminal-state protection (completed/failed quests
 * reject further transitions). Mirrors the currency wallet gate.
 *
 * The test quests deliberately use RequiredLevel = 0 and no Rewards: those
 * two paths deref GetGameInstance(), which is null for a transient-outered
 * subsystem — everything else in the subsystem is pure quest-state logic.
 *
 * Runs headless via:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests Project.Functional Tests.PoF.Quests;Quit" -unattended -nopause -nullrhi -log
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSQuestFlowTest,
	"Project.Functional Tests.PoF.Quests.StageFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	/** Build a quest definition in the transient package (no assets on disk). */
	UARPGQuestDefinition* MakeQuestDef(FName QuestID, bool bAutoComplete, bool bAbandonable, FName PrerequisiteQuestID = NAME_None)
	{
		UARPGQuestDefinition* Def = NewObject<UARPGQuestDefinition>(GetTransientPackage());
		Def->QuestID = QuestID;
		Def->QuestName = FText::FromName(QuestID);
		Def->bAutoComplete = bAutoComplete;
		Def->bAbandonable = bAbandonable;
		Def->PrerequisiteQuestID = PrerequisiteQuestID;
		Def->RequiredLevel = 0; // Must stay 0: level checks need a GameInstance/world.
		return Def;
	}
}

bool FVSQuestFlowTest::RunTest(const FString& /*Parameters*/)
{
	// UARPGQuestSubsystem declares ClassWithin=GameInstance — a transient-package outer
	// trips the CreateNewObject ensure. Give it a bare transient UGameInstance as outer
	// (never Init()'d, so no world/subsystem machinery spins up).
	UGameInstance* OuterGI = NewObject<UGameInstance>(GetTransientPackage());
	if (!TestNotNull(TEXT("Outer game instance created"), OuterGI))
	{
		return false;
	}
	UARPGQuestSubsystem* Quests = NewObject<UARPGQuestSubsystem>(OuterGI);
	if (!TestNotNull(TEXT("Quest subsystem created"), Quests))
	{
		return false;
	}

	UVSQuestEventCounter* Counter = NewObject<UVSQuestEventCounter>(GetTransientPackage());
	if (!TestNotNull(TEXT("Event counter created"), Counter))
	{
		return false;
	}
	Quests->OnQuestAccepted.AddDynamic(Counter, &UVSQuestEventCounter::OnAccepted);
	Quests->OnQuestCompleted.AddDynamic(Counter, &UVSQuestEventCounter::OnCompleted);
	Quests->OnQuestFailed.AddDynamic(Counter, &UVSQuestEventCounter::OnFailed);
	Quests->OnObjectiveUpdated.AddDynamic(Counter, &UVSQuestEventCounter::OnObjectiveUpdated);

	static const FName IntroID(TEXT("VS_Test_Intro"));
	static const FName ChainID(TEXT("VS_Test_Chain"));
	static const FName DoomedID(TEXT("VS_Test_Doomed"));
	static const FName DummyTag(TEXT("VS_TestDummy"));
	static const FName ShardItem(TEXT("VS_TestShard"));

	// Intro quest: 3 stages — Kill 3 dummies, Collect 2 shards, optional TalkTo.
	// Auto-completes when both required stages are done.
	UARPGQuestDefinition* Intro = MakeQuestDef(IntroID, /*bAutoComplete*/ true, /*bAbandonable*/ true);
	{
		FARPGQuestObjective Kill;
		Kill.Type = EARPGObjectiveType::Kill;
		Kill.TargetID = DummyTag;
		Kill.RequiredCount = 3;
		Intro->Objectives.Add(Kill);

		FARPGQuestObjective Collect;
		Collect.Type = EARPGObjectiveType::Collect;
		Collect.TargetID = ShardItem;
		Collect.RequiredCount = 2;
		Intro->Objectives.Add(Collect);

		FARPGQuestObjective Talk;
		Talk.Type = EARPGObjectiveType::TalkTo;
		Talk.TargetID = TEXT("VS_TestNPC");
		Talk.bOptional = true;
		Intro->Objectives.Add(Talk);
	}
	Quests->RegisterQuestDefinition(Intro);

	// Chain quest: gated behind Intro, manual turn-in (bAutoComplete = false).
	UARPGQuestDefinition* Chain = MakeQuestDef(ChainID, /*bAutoComplete*/ false, /*bAbandonable*/ true, IntroID);
	{
		FARPGQuestObjective Manual;
		Manual.Type = EARPGObjectiveType::Manual;
		Manual.TargetID = TEXT("VS_TestRitual");
		Manual.RequiredCount = 1;
		Chain->Objectives.Add(Manual);
	}
	Quests->RegisterQuestDefinition(Chain);

	// Doomed quest: single stage, not abandonable — exercised via the fail path.
	UARPGQuestDefinition* Doomed = MakeQuestDef(DoomedID, /*bAutoComplete*/ true, /*bAbandonable*/ false);
	{
		FARPGQuestObjective Defend;
		Defend.Type = EARPGObjectiveType::Defend;
		Defend.TargetID = TEXT("VS_TestGate");
		Defend.RequiredCount = 1;
		Doomed->Objectives.Add(Defend);
	}
	Quests->RegisterQuestDefinition(Doomed);

	// 1. Registration lookup + untracked quests are Unavailable.
	TestTrue(TEXT("FindQuestDefinition returns the registered intro"), Quests->FindQuestDefinition(IntroID) == Intro);
	TestTrue(TEXT("Untracked quest state is Unavailable"), Quests->GetQuestState(IntroID) == EARPGQuestState::Unavailable);

	// 2. Prerequisite gate: chain quest cannot start before intro completes.
	TestFalse(TEXT("AcceptQuest(chain) rejected while prerequisite incomplete"), Quests->AcceptQuest(ChainID));
	TestFalse(TEXT("Chain quest is not active after rejected accept"), Quests->IsQuestActive(ChainID));

	// 3. Accept intro: Active, per-stage runtime state allocated, no double accept.
	TestTrue(TEXT("AcceptQuest(intro) succeeds"), Quests->AcceptQuest(IntroID));
	TestTrue(TEXT("Intro state is Active"), Quests->GetQuestState(IntroID) == EARPGQuestState::Active);
	TestFalse(TEXT("Re-accepting an active quest rejected"), Quests->AcceptQuest(IntroID));
	const FARPGQuestState* IntroState = Quests->GetQuestRuntimeState(IntroID);
	if (TestNotNull(TEXT("Intro runtime state exists"), IntroState))
	{
		TestEqual(TEXT("Runtime state has one entry per objective (3)"), IntroState->ObjectiveStates.Num(), 3);
	}

	// 4. Pin the active quest for the HUD tracker.
	Quests->PinQuest(IntroID);
	TestEqual(TEXT("Intro is pinned while active"), Quests->GetPinnedQuestID(), IntroID);

	// 5. Event routing: wrong target advances nothing.
	Quests->ReportEvent(EARPGObjectiveType::Kill, TEXT("VS_WrongTag"), 5);
	IntroState = Quests->GetQuestRuntimeState(IntroID);
	TestEqual(TEXT("Kill event with wrong target adds no progress"), IntroState->ObjectiveStates[0].CurrentCount, 0);

	// 6. Stage 1 (Kill 3): partial progress, then overshoot clamps at RequiredCount.
	Quests->ReportEvent(EARPGObjectiveType::Kill, DummyTag, 2);
	IntroState = Quests->GetQuestRuntimeState(IntroID);
	TestEqual(TEXT("Two kills recorded"), IntroState->ObjectiveStates[0].CurrentCount, 2);
	TestFalse(TEXT("Kill stage incomplete at 2/3"), Quests->IsObjectiveComplete(IntroID, 0));

	Quests->ReportEvent(EARPGObjectiveType::Kill, DummyTag, 5);
	IntroState = Quests->GetQuestRuntimeState(IntroID);
	TestEqual(TEXT("Kill count clamps at RequiredCount (3)"), IntroState->ObjectiveStates[0].CurrentCount, 3);
	TestTrue(TEXT("Kill stage complete at 3/3"), Quests->IsObjectiveComplete(IntroID, 0));

	// 7. Stage order rule: quest stays Active until ALL required stages finish.
	TestTrue(TEXT("Intro still Active with collect stage pending"), Quests->GetQuestState(IntroID) == EARPGQuestState::Active);
	TestFalse(TEXT("Not turn-in ready with a required stage pending"), Quests->HasQuestTurnInReady(IntroID));

	// 8. Stage 2 (Collect 2): finishing it auto-completes the quest — the
	//    optional TalkTo stage must NOT block completion (terminal reached).
	Quests->ReportEvent(EARPGObjectiveType::Collect, ShardItem, 2);
	TestTrue(TEXT("Intro auto-completed once required stages done"), Quests->IsQuestComplete(IntroID));
	TestTrue(TEXT("Intro terminal state is Completed"), Quests->GetQuestState(IntroID) == EARPGQuestState::Completed);
	TestEqual(TEXT("OnQuestCompleted fired for the intro"), Counter->LastCompletedQuestID, IntroID);
	TestEqual(TEXT("Completing the pinned quest clears the pin"), Quests->GetPinnedQuestID(), FName(NAME_None));

	// 9. Terminal-state protection: completed quests reject further transitions.
	Quests->ReportEvent(EARPGObjectiveType::Kill, DummyTag, 10);
	TestTrue(TEXT("Events after completion are ignored"), Quests->GetQuestState(IntroID) == EARPGQuestState::Completed);
	Quests->FailQuest(IntroID);
	TestTrue(TEXT("FailQuest on a completed quest rejected"), Quests->GetQuestState(IntroID) == EARPGQuestState::Completed);
	TestFalse(TEXT("AbandonQuest on a completed quest rejected"), Quests->AbandonQuest(IntroID));

	// 10. Prerequisite chain unlocks: chain quest now accepts.
	TestTrue(TEXT("AcceptQuest(chain) succeeds once prerequisite complete"), Quests->AcceptQuest(ChainID));

	// 11. Manual turn-in flow: finished stages leave the quest Active +
	//     turn-in ready (bAutoComplete = false), CompleteQuest closes it.
	Quests->CompleteObjective(ChainID, 0);
	TestTrue(TEXT("Chain stage complete"), Quests->IsObjectiveComplete(ChainID, 0));
	TestTrue(TEXT("Manual quest stays Active with all stages done"), Quests->GetQuestState(ChainID) == EARPGQuestState::Active);
	TestTrue(TEXT("Manual quest reports turn-in ready"), Quests->HasQuestTurnInReady(ChainID));
	Quests->CompleteQuest(ChainID);
	TestTrue(TEXT("Chain quest completed on turn-in"), Quests->IsQuestComplete(ChainID));

	// 12. Fail path + abandon rules on the non-abandonable quest.
	TestTrue(TEXT("AcceptQuest(doomed) succeeds"), Quests->AcceptQuest(DoomedID));
	TestFalse(TEXT("Non-abandonable quest rejects AbandonQuest"), Quests->AbandonQuest(DoomedID));
	Quests->FailQuest(DoomedID);
	TestTrue(TEXT("Doomed quest is Failed"), Quests->IsQuestFailed(DoomedID));
	Quests->AddObjectiveProgress(DoomedID, 0, 1);
	TestFalse(TEXT("Progress on a failed quest rejected"), Quests->IsObjectiveComplete(DoomedID, 0));
	TestFalse(TEXT("Re-accepting a failed quest rejected (state retained)"), Quests->AcceptQuest(DoomedID));

	// 13. Bookkeeping queries reflect the full flow.
	TestEqual(TEXT("Two quests completed"), Quests->GetCompletedQuestIDs().Num(), 2);
	TestEqual(TEXT("One quest failed"), Quests->GetFailedQuestIDs().Num(), 1);
	TestEqual(TEXT("No quests remain active"), Quests->GetActiveQuestIDs().Num(), 0);

	// 14. Telemetry hooks fired for the real transitions.
	TestEqual(TEXT("OnQuestAccepted fired 3 times"), Counter->AcceptedCount, 3);
	TestEqual(TEXT("OnQuestCompleted fired 2 times"), Counter->CompletedCount, 2);
	TestEqual(TEXT("OnQuestFailed fired once"), Counter->FailedCount, 1);
	TestTrue(
		FString::Printf(TEXT("OnObjectiveUpdated fired per progress tick (got %d, expected >= 3)"), Counter->ObjectiveUpdateCount),
		Counter->ObjectiveUpdateCount >= 3);

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

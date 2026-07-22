#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Dialogue/ARPGDialogueTree.h"
#include "Dialogue/ARPGDuelIntroDialogue.h"
#include "UObject/Package.h"

/**
 * Dialog-trees gate — the Duel Challenge tree (dialog-trees catalog:
 * dialog-duel-intro). Builds the production tree (ARPGDuelIntroDialogue::Populate)
 * on a transient UARPGDialogueTree and asserts the SOR Branch Graph contract:
 * valid tree (no dangling indices), root with exactly two player choices
 * (defy / silent ignite), and BOTH branches reaching a terminal (combat starts —
 * the duel is not skippable). Headless:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests Project.Functional Tests.PoF.DialogTrees.DuelIntroConfig;Quit" -unattended -nopause -nullrhi -log
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSDialogDuelIntroTest,
	"Project.Functional Tests.PoF.DialogTrees.DuelIntroConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSDialogDuelIntroTest::RunTest(const FString& /*Parameters*/)
{
	UARPGDialogueTree* Tree = NewObject<UARPGDialogueTree>(GetTransientPackage());
	if (!TestNotNull(TEXT("Transient dialogue tree instantiates"), Tree))
	{
		return false;
	}
	ARPGDuelIntroDialogue::Populate(*Tree);

	// Structural integrity — the same contract the app-side checker enforces
	// (edge integrity + terminal marking).
	TestTrue(TEXT("Tree validates (no dangling node references)"), Tree->IsValid());
	TestEqual(TEXT("Three beats: challenge + two responses"), Tree->Nodes.Num(), 3);

	const FARPGDialogueNode* Root = Tree->GetNode(Tree->StartNodeIndex);
	if (!TestNotNull(TEXT("Root node resolves"), Root))
	{
		return false;
	}
	TestEqual(TEXT("Malgrave speaks the challenge"), Root->SpeakerID, FName(TEXT("Malgrave")));
	TestEqual(TEXT("Root offers exactly two player choices"), Root->Choices.Num(), 2);

	// Both branches must reach combat (a terminal node) in one response beat.
	for (const FARPGDialogueChoice& Choice : Root->Choices)
	{
		const FARPGDialogueNode* Response = Tree->GetNode(Choice.NextNodeIndex);
		if (!TestNotNull(TEXT("Choice leads to a response beat"), Response))
		{
			continue;
		}
		TestTrue(TEXT("Response is Malgrave's"), Response->SpeakerID == FName(TEXT("Malgrave")));
		TestTrue(TEXT("Response has no further choices (pre-combat beat, not a quest)"),
			Response->Choices.Num() == 0);
		TestEqual(TEXT("Response terminates the dialog (combat starts)"),
			Response->NextNodeIndex, -1);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

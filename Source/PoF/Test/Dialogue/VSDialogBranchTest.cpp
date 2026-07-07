#include "Test/Dialogue/VSDialogBranchTest.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Dialogue/ARPGDialogueTree.h"
#include "Dialogue/ARPGDialogueTypes.h"

/**
 * Dialog Trees Test Gate — branch integrity on the index-addressed tree.
 *
 * Pure logic gate (no PIE/world/map needed — safe under shared-tree
 * concurrency). Builds a 5-node in-memory UARPGDialogueTree with a real
 * branch (greeting -> 2 choices -> quest path / shop path -> shared
 * farewell -> end) and asserts the integrity rules the data model supports:
 * every NextNodeIndex / choice reference resolves via GetNode, a terminal
 * (NextNodeIndex == -1, no choices) is reachable from StartNodeIndex, no
 * node loops to itself, choice lookup lands on the intended node, and
 * IsValid() genuinely rejects dangling references / bad start indices
 * (so a regression in the validator fails the gate, not just the fixture).
 *
 * Runs headless via:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests Project.Functional Tests.PoF.DialogTrees;Quit" -unattended -nopause -nullrhi -log
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSDialogBranchTest,
	"Project.Functional Tests.PoF.DialogTrees.BranchIntegrity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	/** Build the fixture tree: 0 greeting (2 choices) -> 1 quest / 2 shop -> 3 farewell -> 4 end. */
	UARPGDialogueTree* BuildBranchTree()
	{
		UARPGDialogueTree* Tree = NewObject<UARPGDialogueTree>(GetTransientPackage());
		Tree->DialogueID = TEXT("VS_BranchIntegrity");
		Tree->DialogueName = FText::FromString(TEXT("Branch Integrity Fixture"));
		Tree->StartNodeIndex = 0;
		Tree->Nodes.SetNum(5);

		// Node 0 — greeting with a real branch (two choices, no auto-advance).
		Tree->Nodes[0].SpeakerID = TEXT("Innkeeper");
		Tree->Nodes[0].DialogueText = FText::FromString(TEXT("Welcome, traveler. What brings you?"));
		Tree->Nodes[0].NextNodeIndex = -1; // choices drive navigation here
		FARPGDialogueChoice QuestChoice;
		QuestChoice.ChoiceText = FText::FromString(TEXT("I seek work."));
		QuestChoice.NextNodeIndex = 1;
		FARPGDialogueChoice ShopChoice;
		ShopChoice.ChoiceText = FText::FromString(TEXT("Show me your wares."));
		ShopChoice.NextNodeIndex = 2;
		Tree->Nodes[0].Choices.Add(QuestChoice);
		Tree->Nodes[0].Choices.Add(ShopChoice);

		// Node 1 — quest branch, auto-advances to the shared farewell.
		Tree->Nodes[1].SpeakerID = TEXT("Innkeeper");
		Tree->Nodes[1].DialogueText = FText::FromString(TEXT("Rats in the cellar. Clear them out."));
		Tree->Nodes[1].NextNodeIndex = 3;

		// Node 2 — shop branch, auto-advances to the shared farewell.
		Tree->Nodes[2].SpeakerID = TEXT("Innkeeper");
		Tree->Nodes[2].DialogueText = FText::FromString(TEXT("Potions, two gold apiece."));
		Tree->Nodes[2].NextNodeIndex = 3;

		// Node 3 — shared farewell (branches converge), one choice to the end.
		Tree->Nodes[3].SpeakerID = TEXT("Innkeeper");
		Tree->Nodes[3].DialogueText = FText::FromString(TEXT("Anything else?"));
		Tree->Nodes[3].NextNodeIndex = -1;
		FARPGDialogueChoice LeaveChoice;
		LeaveChoice.ChoiceText = FText::FromString(TEXT("No, farewell."));
		LeaveChoice.NextNodeIndex = 4;
		Tree->Nodes[3].Choices.Add(LeaveChoice);

		// Node 4 — terminal (no choices, NextNodeIndex -1 ends the dialogue).
		Tree->Nodes[4].SpeakerID = TEXT("Innkeeper");
		Tree->Nodes[4].DialogueText = FText::FromString(TEXT("Safe travels."));
		Tree->Nodes[4].NextNodeIndex = -1;

		return Tree;
	}
}

bool FVSDialogBranchTest::RunTest(const FString& /*Parameters*/)
{
	UARPGDialogueTree* Tree = BuildBranchTree();
	if (!TestNotNull(TEXT("Dialogue tree created"), Tree))
	{
		return false;
	}

	// 1. Baseline: a well-formed 5-node branch passes the tree's own validator.
	TestEqual(TEXT("Fixture has 5 nodes"), Tree->Nodes.Num(), 5);
	TestTrue(TEXT("Well-formed branch tree passes IsValid()"), Tree->IsValid());

	// 2. Root lookup: GetNode(StartNodeIndex) returns the greeting node.
	const FARPGDialogueNode* Root = Tree->GetNode(Tree->StartNodeIndex);
	if (!TestNotNull(TEXT("GetNode(StartNodeIndex) resolves"), Root))
	{
		return false;
	}
	TestEqual(TEXT("Root speaker is the Innkeeper"), Root->SpeakerID, FName(TEXT("Innkeeper")));
	TestEqual(TEXT("Root offers exactly 2 choices (real branch)"), Root->Choices.Num(), 2);

	// 3. Every reference resolves: NextNodeIndex and every choice on every node
	//    either ends the dialogue (-1) or lands on an existing node.
	for (int32 NodeIdx = 0; NodeIdx < Tree->Nodes.Num(); ++NodeIdx)
	{
		const FARPGDialogueNode& Node = Tree->Nodes[NodeIdx];
		TestTrue(
			FString::Printf(TEXT("Node %d NextNodeIndex (%d) resolves or ends"), NodeIdx, Node.NextNodeIndex),
			Node.NextNodeIndex == -1 || Tree->GetNode(Node.NextNodeIndex) != nullptr);

		for (int32 ChoiceIdx = 0; ChoiceIdx < Node.Choices.Num(); ++ChoiceIdx)
		{
			const int32 Target = Node.Choices[ChoiceIdx].NextNodeIndex;
			TestTrue(
				FString::Printf(TEXT("Node %d choice %d target (%d) resolves or ends"), NodeIdx, ChoiceIdx, Target),
				Target == -1 || Tree->GetNode(Target) != nullptr);

			// 4. No self-loop: a choice must never point back at its own node.
			TestTrue(
				FString::Printf(TEXT("Node %d choice %d does not self-loop"), NodeIdx, ChoiceIdx),
				Target != NodeIdx);
		}

		// 4b. No self-loop via auto-advance either.
		TestTrue(
			FString::Printf(TEXT("Node %d does not auto-advance to itself"), NodeIdx),
			Node.NextNodeIndex != NodeIdx);
	}

	// 5. Choice lookup returns the RIGHT node: taking each branch from the root
	//    must land on the intended line, and both branches converge on farewell.
	const FARPGDialogueNode* QuestNode = Tree->GetNode(Root->Choices[0].NextNodeIndex);
	if (TestNotNull(TEXT("Quest choice resolves to a node"), QuestNode))
	{
		TestEqual(
			TEXT("Quest choice lands on the cellar-rats line"),
			QuestNode->DialogueText.ToString(),
			FString(TEXT("Rats in the cellar. Clear them out.")));
		TestEqual(TEXT("Quest branch converges on farewell (node 3)"), QuestNode->NextNodeIndex, 3);
	}
	const FARPGDialogueNode* ShopNode = Tree->GetNode(Root->Choices[1].NextNodeIndex);
	if (TestNotNull(TEXT("Shop choice resolves to a node"), ShopNode))
	{
		TestEqual(
			TEXT("Shop choice lands on the potions line"),
			ShopNode->DialogueText.ToString(),
			FString(TEXT("Potions, two gold apiece.")));
		TestEqual(TEXT("Shop branch converges on farewell (node 3)"), ShopNode->NextNodeIndex, 3);
	}

	// 6. Terminal reachability: walk every edge from the root (BFS) and require
	//    a reachable node that ends the dialogue (no choices, NextNodeIndex -1).
	{
		TSet<int32> Visited;
		TArray<int32> Frontier;
		Frontier.Add(Tree->StartNodeIndex);
		bool bTerminalReachable = false;
		while (Frontier.Num() > 0)
		{
			const int32 Index = Frontier.Pop();
			if (Visited.Contains(Index))
			{
				continue;
			}
			Visited.Add(Index);
			const FARPGDialogueNode* Node = Tree->GetNode(Index);
			if (!Node)
			{
				continue;
			}
			if (Node->Choices.Num() == 0 && Node->NextNodeIndex == -1)
			{
				bTerminalReachable = true;
			}
			if (Node->NextNodeIndex >= 0)
			{
				Frontier.Add(Node->NextNodeIndex);
			}
			for (const FARPGDialogueChoice& Choice : Node->Choices)
			{
				if (Choice.NextNodeIndex >= 0)
				{
					Frontier.Add(Choice.NextNodeIndex);
				}
			}
		}
		TestTrue(TEXT("A terminal node is reachable from the root"), bTerminalReachable);
		TestEqual(TEXT("Every node is reachable from the root (no orphans)"), Visited.Num(), Tree->Nodes.Num());
	}

	// 7. Out-of-range lookup is safe: GetNode never returns garbage.
	TestNull(TEXT("GetNode(-1) returns nullptr"), Tree->GetNode(-1));
	TestNull(TEXT("GetNode past the end returns nullptr"), Tree->GetNode(Tree->Nodes.Num()));

	// 8. Validator teeth: a dangling choice reference must flip IsValid() to false.
	{
		UARPGDialogueTree* Broken = BuildBranchTree();
		Broken->Nodes[0].Choices[0].NextNodeIndex = 99; // dangling
		TestFalse(TEXT("Dangling choice reference fails IsValid()"), Broken->IsValid());
	}

	// 9. Validator teeth: a dangling auto-advance reference must also fail.
	{
		UARPGDialogueTree* Broken = BuildBranchTree();
		Broken->Nodes[1].NextNodeIndex = 42; // dangling
		TestFalse(TEXT("Dangling NextNodeIndex fails IsValid()"), Broken->IsValid());
	}

	// 10. Validator teeth: bad start index / empty tree are rejected.
	{
		UARPGDialogueTree* Broken = BuildBranchTree();
		Broken->StartNodeIndex = 7; // out of range
		TestFalse(TEXT("Out-of-range StartNodeIndex fails IsValid()"), Broken->IsValid());

		UARPGDialogueTree* Empty = NewObject<UARPGDialogueTree>(GetTransientPackage());
		TestFalse(TEXT("Empty tree fails IsValid()"), Empty->IsValid());
	}

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

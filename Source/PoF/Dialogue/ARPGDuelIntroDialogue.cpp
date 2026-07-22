#include "Dialogue/ARPGDuelIntroDialogue.h"
#include "Dialogue/ARPGDialogueTree.h"

namespace ARPGDuelIntroDialogue
{

void Populate(UARPGDialogueTree& Tree)
{
	Tree.Nodes.Reset();
	Tree.StartNodeIndex = 0;

	const FName Malgrave(TEXT("Malgrave"));
	const FText MalgraveName = FText::FromString(TEXT("Darth Malgrave"));

	// 0 — challenge (root): two player choices, mirroring the SOR branch graph.
	{
		FARPGDialogueNode Node;
		Node.SpeakerID = Malgrave;
		Node.SpeakerDisplayName = MalgraveName;
		Node.DialogueText = FText::FromString(TEXT("A Jedi. Alone. The Force has a sense of humor."));

		FARPGDialogueChoice Defy;
		Defy.ChoiceText = FText::FromString(TEXT("Three of you, one of me. Hardly fair - for you."));
		Defy.NextNodeIndex = 1; // → laugh

		FARPGDialogueChoice Silent;
		Silent.ChoiceText = FText::FromString(TEXT("[Ignite saber silently]"));
		Silent.NextNodeIndex = 2; // → respect

		Node.Choices.Add(Defy);
		Node.Choices.Add(Silent);
		Tree.Nodes.Add(Node);
	}

	// 1 — laugh [combat]: terminal (SOR END sentinel → NextNodeIndex -1).
	{
		FARPGDialogueNode Node;
		Node.SpeakerID = Malgrave;
		Node.SpeakerDisplayName = MalgraveName;
		Node.DialogueText = FText::FromString(TEXT("Spirit! I will carve it out of you slowly."));
		Node.NextNodeIndex = -1;
		Tree.Nodes.Add(Node);
	}

	// 2 — respect [combat]: terminal.
	{
		FARPGDialogueNode Node;
		Node.SpeakerID = Malgrave;
		Node.SpeakerDisplayName = MalgraveName;
		Node.DialogueText = FText::FromString(TEXT("No words. Good. Blades speak truer."));
		Node.NextNodeIndex = -1;
		Tree.Nodes.Add(Node);
	}
}

} // namespace ARPGDuelIntroDialogue

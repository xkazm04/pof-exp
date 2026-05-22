#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ARPGDialogueTypes.h"
#include "ARPGDialogueTree.generated.h"

/**
 * Data asset defining a branching dialogue tree.
 * Contains an ordered array of dialogue nodes with choices, conditions, and actions.
 * Designers create these in the editor and assign them to NPCs.
 */
UCLASS(BlueprintType)
class POF_API UARPGDialogueTree : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Unique identifier for this dialogue. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FName DialogueID;

	/** Display name (for editor/debug). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	FText DialogueName;

	/** All nodes in the tree, addressed by array index. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	TArray<FARPGDialogueNode> Nodes;

	/** Index of the starting node (default 0). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	int32 StartNodeIndex = 0;

	/** Whether this dialogue can only be triggered once per playthrough. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue")
	bool bOneShot = false;

	// UPrimaryDataAsset interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("DialogueTree"), GetFName());
	}

	/** Get a node by index, or nullptr if out of range. */
	const FARPGDialogueNode* GetNode(int32 Index) const
	{
		return Nodes.IsValidIndex(Index) ? &Nodes[Index] : nullptr;
	}

	/** Validate the tree (check for dangling references). Editor utility. */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsValid() const
	{
		if (Nodes.Num() == 0) return false;
		if (!Nodes.IsValidIndex(StartNodeIndex)) return false;

		for (const FARPGDialogueNode& Node : Nodes)
		{
			if (Node.NextNodeIndex >= 0 && !Nodes.IsValidIndex(Node.NextNodeIndex)) return false;
			for (const FARPGDialogueChoice& Choice : Node.Choices)
			{
				if (Choice.NextNodeIndex >= 0 && !Nodes.IsValidIndex(Choice.NextNodeIndex)) return false;
			}
		}
		return true;
	}
};

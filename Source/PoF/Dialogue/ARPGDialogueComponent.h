#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ARPGDialogueTypes.h"
#include "ARPGDialogueComponent.generated.h"

class UARPGDialogueTree;
class AARPGPlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueStarted, UARPGDialogueTree*, Dialogue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogueEnded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueNodeEntered, int32, NodeIndex, const FARPGDialogueNode&, Node);

/**
 * Component that manages dialogue state for an NPC.
 * Holds a list of dialogue trees (checked in order for conditions),
 * tracks the current conversation state, and evaluates conditions/actions.
 */
UCLASS(ClassGroup = "Dialogue", meta = (BlueprintSpawnableComponent))
class POF_API UARPGDialogueComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGDialogueComponent();

	/** Dialogue trees to check, in priority order. First valid one is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TArray<TObjectPtr<UARPGDialogueTree>> DialogueTrees;

	/** Start a conversation with the interacting player. Returns true if dialogue started. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	bool StartDialogue(AARPGPlayerCharacter* Player);

	/** Advance the conversation by selecting a choice. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void SelectChoice(int32 ChoiceIndex);

	/** Skip/advance a node with no choices (click to continue). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void AdvanceDialogue();

	/** End the current conversation. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void EndDialogue();

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	bool IsInDialogue() const { return bInDialogue; }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	int32 GetCurrentNodeIndex() const { return CurrentNodeIndex; }

	UFUNCTION(BlueprintPure, Category = "Dialogue")
	UARPGDialogueTree* GetActiveDialogue() const { return ActiveDialogue; }

	/** Get the current node (nullptr if not in dialogue). */
	UFUNCTION(BlueprintPure, Category = "Dialogue")
	const FARPGDialogueNode& GetCurrentNode() const;

	/** Get choices available at the current node (filtered by conditions). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	TArray<FARPGDialogueChoice> GetAvailableChoices() const;

	/** Check if a one-shot dialogue has already been used. */
	bool HasUsedOneShot(FName DialogueID) const { return UsedOneShotDialogues.Contains(DialogueID); }

	/** Public condition check for NPC HasDialogueForPlayer. */
	bool CheckConditions(const TArray<FARPGDialogueCondition>& Conditions, AARPGPlayerCharacter* Player) const
	{
		return EvaluateConditions(Conditions, Player);
	}

	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
	FOnDialogueStarted OnDialogueStarted;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
	FOnDialogueEnded OnDialogueEnded;

	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
	FOnDialogueNodeEntered OnDialogueNodeEntered;

private:
	UPROPERTY()
	TObjectPtr<UARPGDialogueTree> ActiveDialogue;

	UPROPERTY()
	TWeakObjectPtr<AARPGPlayerCharacter> DialoguePlayer;

	int32 CurrentNodeIndex = -1;
	bool bInDialogue = false;

	/** Set of one-shot dialogues already used (by DialogueID). */
	UPROPERTY()
	TSet<FName> UsedOneShotDialogues;

	/** Select the best dialogue tree based on conditions. */
	UARPGDialogueTree* SelectDialogue(AARPGPlayerCharacter* Player) const;

	/** Navigate to a specific node index. -1 ends dialogue. */
	void GoToNode(int32 NodeIndex);

	/** Evaluate a single condition against the current player. */
	bool EvaluateCondition(const FARPGDialogueCondition& Condition, AARPGPlayerCharacter* Player) const;

	/** Evaluate all conditions (AND logic). */
	bool EvaluateConditions(const TArray<FARPGDialogueCondition>& Conditions, AARPGPlayerCharacter* Player) const;

	/** Execute a single action. */
	void ExecuteAction(const FARPGDialogueAction& Action, AARPGPlayerCharacter* Player);

	/** Execute all actions in order. */
	void ExecuteActions(const TArray<FARPGDialogueAction>& Actions, AARPGPlayerCharacter* Player);

	static const FARPGDialogueNode EmptyNode;
};

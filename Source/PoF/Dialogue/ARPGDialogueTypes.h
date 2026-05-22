#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ARPGDialogueTypes.generated.h"

/** Condition type for dialogue branching */
UENUM(BlueprintType)
enum class EARPGDialogueConditionType : uint8
{
	None,
	HasQuest,
	QuestActive,
	QuestComplete,
	QuestObjectiveComplete,
	HasItem,
	PlayerLevelMin,
	HasGameplayTag
};

/** Action type triggered by dialogue choices */
UENUM(BlueprintType)
enum class EARPGDialogueActionType : uint8
{
	None,
	GiveQuest,
	CompleteQuest,
	GiveItem,
	RemoveItem,
	FailQuest,
	GiveXP,
	AddGameplayTag,
	RemoveGameplayTag
};

/** A condition that must be met for a choice/node to be available */
USTRUCT(BlueprintType)
struct FARPGDialogueCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	EARPGDialogueConditionType Type = EARPGDialogueConditionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName ParamName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	int32 ParamInt = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FGameplayTag ParamTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (Tooltip = "If true, condition is inverted (NOT)"))
	bool bInvert = false;
};

/** An action triggered when a choice is selected */
USTRUCT(BlueprintType)
struct FARPGDialogueAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	EARPGDialogueActionType Type = EARPGDialogueActionType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName ParamName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	int32 ParamInt = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FGameplayTag ParamTag;
};

/** A player choice in a dialogue node */
USTRUCT(BlueprintType)
struct FARPGDialogueChoice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText ChoiceText;

	/** Index of the node this choice leads to. -1 = end dialogue. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	int32 NextNodeIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TArray<FARPGDialogueCondition> Conditions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TArray<FARPGDialogueAction> Actions;
};

/** A single node in the dialogue tree */
USTRUCT(BlueprintType)
struct FARPGDialogueNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName SpeakerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText SpeakerDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText DialogueText;

	/** Portrait texture for the speaker (optional). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TSoftObjectPtr<UTexture2D> SpeakerPortrait;

	/** Player choices. If empty, auto-advance to NextNodeIndex. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TArray<FARPGDialogueChoice> Choices;

	/** Auto-advance target when no choices. -1 = end dialogue. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	int32 NextNodeIndex = -1;

	/** Conditions that must all pass for this node to be entered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TArray<FARPGDialogueCondition> EntryConditions;

	/** Actions executed when this node is entered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TArray<FARPGDialogueAction> EntryActions;
};

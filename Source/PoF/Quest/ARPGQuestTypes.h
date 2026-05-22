#pragma once

#include "CoreMinimal.h"
#include "ARPGQuestTypes.generated.h"

/** Quest category for organization in the quest log */
UENUM(BlueprintType)
enum class EARPGQuestCategory : uint8
{
	Main,
	Side,
	Bounty,
	Exploration,
	Crafting
};

/** How an objective is tracked */
UENUM(BlueprintType)
enum class EARPGObjectiveType : uint8
{
	/** Manually completed via script/dialogue action */
	Manual,
	/** Kill N enemies of a certain type (Tag-based) */
	Kill,
	/** Collect N of a specific item */
	Collect,
	/** Reach a location (trigger volume) */
	Location,
	/** Talk to a specific NPC */
	TalkTo,
	/** Escort an NPC to a destination */
	Escort,
	/** Defend a point for a duration or against waves */
	Defend
};

/** State of a quest */
UENUM(BlueprintType)
enum class EARPGQuestState : uint8
{
	Unavailable,
	Available,
	Active,
	Completed,
	Failed
};

/** A single quest objective */
USTRUCT(BlueprintType)
struct FARPGQuestObjective
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	EARPGObjectiveType Type = EARPGObjectiveType::Manual;

	/** Target identifier: enemy tag name, item ID, location name, or NPC ID. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName TargetID;

	/** Required count (for Kill/Collect objectives). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest", meta = (ClampMin = "1"))
	int32 RequiredCount = 1;

	/** Whether this objective is optional. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	bool bOptional = false;
};

/** Runtime state of a single objective */
USTRUCT(BlueprintType)
struct FARPGQuestObjectiveState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
	int32 CurrentCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
	bool bComplete = false;
};

/** Runtime state for an active/completed quest */
USTRUCT(BlueprintType)
struct FARPGQuestState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
	FName QuestID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
	EARPGQuestState State = EARPGQuestState::Active;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FARPGQuestObjectiveState> ObjectiveStates;
};

/** Reward entry */
USTRUCT(BlueprintType)
struct FARPGQuestReward
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 XP = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 ItemQuantity = 1;
};

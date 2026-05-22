#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ARPGQuestTypes.h"
#include "ARPGQuestDefinition.generated.h"

/**
 * Data asset defining a quest: objectives, rewards, and metadata.
 * Designers create these in the editor and reference them by QuestID.
 */
UCLASS(BlueprintType)
class POF_API UARPGQuestDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Unique quest identifier (used by subsystem lookups). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName QuestID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FText QuestName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	EARPGQuestCategory Category = EARPGQuestCategory::Side;

	/** Minimum player level to accept this quest. 0 = no requirement. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (ClampMin = "0"))
	int32 RequiredLevel = 0;

	/** Quest that must be completed before this one becomes available. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName PrerequisiteQuestID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FARPGQuestObjective> Objectives;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TArray<FARPGQuestReward> Rewards;

	/** If true, quest auto-completes when all non-optional objectives are done. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	bool bAutoComplete = true;

	/** If true, quest can be abandoned by the player. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	bool bAbandonable = true;

	// UPrimaryDataAsset interface
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("QuestDefinition"), GetFName());
	}
};

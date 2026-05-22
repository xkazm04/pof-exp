#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ARPGQuestTypes.h"
#include "ARPGQuestSubsystem.generated.h"

class UARPGQuestDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestAccepted, FName, QuestID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestCompleted, FName, QuestID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestFailed, FName, QuestID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestAbandoned, FName, QuestID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnObjectiveUpdated, FName, QuestID, int32, ObjectiveIndex, int32, NewCount);

/**
 * Game-instance subsystem that manages all quest state.
 * Persists across level transitions. Provides quest accept/complete/fail
 * and objective progress tracking with event delegates for UI binding.
 */
UCLASS()
class POF_API UARPGQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// =====================================================================
	// Quest Registration
	// =====================================================================

	/** Register a quest definition so it can be referenced by QuestID. */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void RegisterQuestDefinition(UARPGQuestDefinition* Definition);

	/** Find a registered quest definition by ID. */
	UFUNCTION(BlueprintPure, Category = "Quest")
	UARPGQuestDefinition* FindQuestDefinition(FName QuestID) const;

	// =====================================================================
	// Quest Lifecycle
	// =====================================================================

	/** Accept a quest (move to Active). Returns false if prerequisites not met. */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool AcceptQuest(FName QuestID);

	/** Manually complete a quest (bypasses objective checks). */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void CompleteQuest(FName QuestID);

	/** Fail a quest. */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void FailQuest(FName QuestID);

	/** Abandon an active quest (removes it from active list). */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool AbandonQuest(FName QuestID);

	// =====================================================================
	// Objective Progress
	// =====================================================================

	/** Add progress to a specific objective. */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void AddObjectiveProgress(FName QuestID, int32 ObjectiveIndex, int32 Amount = 1);

	/** Complete a specific objective directly. */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void CompleteObjective(FName QuestID, int32 ObjectiveIndex);

	/** Report an event (kill, collect, location, talk) — auto-advances matching objectives. */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void ReportEvent(EARPGObjectiveType EventType, FName TargetID, int32 Count = 1);

	// =====================================================================
	// Query
	// =====================================================================

	UFUNCTION(BlueprintPure, Category = "Quest")
	bool IsQuestActive(FName QuestID) const;

	UFUNCTION(BlueprintPure, Category = "Quest")
	bool IsQuestComplete(FName QuestID) const;

	UFUNCTION(BlueprintPure, Category = "Quest")
	bool IsQuestFailed(FName QuestID) const;

	UFUNCTION(BlueprintPure, Category = "Quest")
	bool IsObjectiveComplete(FName QuestID, int32 ObjectiveIndex) const;

	UFUNCTION(BlueprintPure, Category = "Quest")
	EARPGQuestState GetQuestState(FName QuestID) const;

	/** Get the runtime state for a quest. Use the non-Blueprint version for pointer access. */
	const FARPGQuestState* GetQuestRuntimeState(FName QuestID) const;

	/** Blueprint-friendly version that copies the state. Returns false if quest not found. */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool GetQuestRuntimeStateCopy(FName QuestID, FARPGQuestState& OutState) const;

	UFUNCTION(BlueprintCallable, Category = "Quest")
	TArray<FName> GetActiveQuestIDs() const;

	UFUNCTION(BlueprintCallable, Category = "Quest")
	TArray<FName> GetCompletedQuestIDs() const;

	UFUNCTION(BlueprintCallable, Category = "Quest")
	TArray<FName> GetFailedQuestIDs() const;

	/** Check if the player has an active or completed quest matching the given ID. Used for NPC indicators. */
	UFUNCTION(BlueprintPure, Category = "Quest")
	bool HasQuestTurnInReady(FName QuestID) const;

	// =====================================================================
	// Pinned Quest (for HUD tracker focus)
	// =====================================================================

	/** Pin a quest to show prominently on the HUD tracker. */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void PinQuest(FName QuestID);

	/** Unpin the current pinned quest. */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void UnpinQuest();

	UFUNCTION(BlueprintPure, Category = "Quest")
	FName GetPinnedQuestID() const { return PinnedQuestID; }

	UFUNCTION(BlueprintPure, Category = "Quest")
	bool HasPinnedQuest() const { return !PinnedQuestID.IsNone(); }

	// =====================================================================
	// Save/Load
	// =====================================================================

	UFUNCTION(BlueprintCallable, Category = "Quest")
	TArray<FARPGQuestState> GetAllQuestStates() const;

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void RestoreQuestStates(const TArray<FARPGQuestState>& States, FName InPinnedQuestID = NAME_None);

	// =====================================================================
	// Delegates
	// =====================================================================

	UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
	FOnQuestAccepted OnQuestAccepted;

	UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
	FOnQuestCompleted OnQuestCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
	FOnQuestFailed OnQuestFailed;

	UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
	FOnQuestAbandoned OnQuestAbandoned;

	UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
	FOnObjectiveUpdated OnObjectiveUpdated;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestPinChanged, FName, QuestID);

	UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
	FOnQuestPinChanged OnQuestPinChanged;

private:
	/** Registered quest definitions (QuestID -> Definition). */
	UPROPERTY()
	TMap<FName, TObjectPtr<UARPGQuestDefinition>> QuestDefinitions;

	/** Runtime quest states (QuestID -> State). */
	TMap<FName, FARPGQuestState> QuestStates;

	FName PinnedQuestID;

	/** Check if all required objectives are complete and auto-complete if appropriate. */
	void CheckAutoComplete(FName QuestID);

	/** Grant quest rewards (XP, items) to the first local player. */
	void GrantRewards(FName QuestID);

	/** Notify quests whose prerequisite just completed to become available. */
	void NotifyPrerequisiteCompleted(FName CompletedQuestID);
};

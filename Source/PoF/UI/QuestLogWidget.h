#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QuestLogWidget.generated.h"

class UARPGQuestSubsystem;
class UARPGQuestDefinition;
class UVerticalBox;
class UTextBlock;
class UButton;
class UBorder;
class UScrollBox;

/**
 * Full-screen quest log with Active/Completed tabs.
 * Shows quest names, descriptions, objectives with progress, and rewards.
 */
UCLASS()
class POF_API UQuestLogWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void BindToQuestSubsystem(UARPGQuestSubsystem* QuestSubsystem);

	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void RefreshDisplay();

	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void ShowActiveTab();

	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void ShowCompletedTab();

	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void ShowFailedTab();

	/** Select a quest to show details. */
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void SelectQuest(FName QuestID);

	/** Abandon the currently selected quest. */
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void AbandonSelectedQuest();

	/** Pin the currently selected quest for HUD tracking. */
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void PinSelectedQuest();

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY()
	TWeakObjectPtr<UARPGQuestSubsystem> BoundSubsystem;

	// Layout elements
	UPROPERTY()
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY()
	TObjectPtr<UButton> ActiveTabButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> ActiveTabText;

	UPROPERTY()
	TObjectPtr<UButton> CompletedTabButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> CompletedTabText;

	UPROPERTY()
	TObjectPtr<UButton> FailedTabButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> FailedTabText;

	UPROPERTY()
	TObjectPtr<UScrollBox> QuestListScroll;

	UPROPERTY()
	TObjectPtr<UVerticalBox> QuestListContainer;

	// Detail panel
	UPROPERTY()
	TObjectPtr<UVerticalBox> DetailPanel;

	UPROPERTY()
	TObjectPtr<UTextBlock> DetailQuestName;

	UPROPERTY()
	TObjectPtr<UTextBlock> DetailQuestDescription;

	UPROPERTY()
	TObjectPtr<UVerticalBox> DetailObjectivesContainer;

	UPROPERTY()
	TObjectPtr<UVerticalBox> DetailRewardsContainer;

	UPROPERTY()
	TObjectPtr<UButton> PinButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> PinButtonText;

	UPROPERTY()
	TObjectPtr<UButton> AbandonButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> AbandonButtonText;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> QuestListButtons;

	/** Parallel array: QuestListButtons[i] corresponds to QuestListIDs[i]. */
	TArray<FName> QuestListIDs;

	/** 0 = active, 1 = completed, 2 = failed */
	int32 ActiveTabIndex = 0;
	FName SelectedQuestID;

	void BuildLayout();
	void PopulateQuestList();
	void PopulateDetail();
	void CreateQuestListEntry(FName QuestID, const FText& QuestName, bool bIsActive);

	void UpdateTabStyles();

	UFUNCTION()
	void OnActiveTabClicked();

	UFUNCTION()
	void OnCompletedTabClicked();

	UFUNCTION()
	void OnFailedTabClicked();

	UFUNCTION()
	void OnPinClicked();

	UFUNCTION()
	void OnAbandonClicked();

	/** Single handler for all quest list buttons — resolves via hover check. */
	UFUNCTION()
	void OnQuestListButtonClicked();

	UFUNCTION()
	void OnQuestAccepted(FName QuestID);

	UFUNCTION()
	void OnQuestCompleted(FName QuestID);

	UFUNCTION()
	void OnQuestFailed(FName QuestID);

	UFUNCTION()
	void OnObjectiveUpdated(FName QuestID, int32 ObjectiveIndex, int32 NewCount);
};

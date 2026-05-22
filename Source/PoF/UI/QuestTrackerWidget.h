#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "QuestTrackerWidget.generated.h"

class UARPGQuestSubsystem;
class UVerticalBox;
class UTextBlock;

/**
 * HUD widget that displays active quest objectives.
 * Auto-refreshes when quest state changes.
 */
UCLASS()
class POF_API UQuestTrackerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Bind to the quest subsystem. Call from HUD BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void BindToQuestSubsystem(UARPGQuestSubsystem* QuestSubsystem);

	/** Force a full refresh of displayed quests. */
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void RefreshDisplay();

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY()
	TWeakObjectPtr<UARPGQuestSubsystem> BoundSubsystem;

	UPROPERTY()
	TObjectPtr<UVerticalBox> QuestListContainer;

	UPROPERTY()
	TObjectPtr<UTextBlock> HeaderText;

	/** Maximum quests shown at once. */
	int32 MaxDisplayedQuests = 3;

	void BuildLayout();

	UFUNCTION()
	void OnQuestAccepted(FName QuestID);

	UFUNCTION()
	void OnQuestCompleted(FName QuestID);

	UFUNCTION()
	void OnQuestFailed(FName QuestID);

	UFUNCTION()
	void OnObjectiveUpdated(FName QuestID, int32 ObjectiveIndex, int32 NewCount);

	UFUNCTION()
	void OnQuestPinChanged(FName QuestID);
};

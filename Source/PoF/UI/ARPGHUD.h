#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ARPGHUD.generated.h"

class UARPGHUDWidget;
class UAbilityBarWidget;
class UAbilityUnlockWidget;
class UInventoryScreenWidget;
class UCharacterStatsWidget;
class UAttributeAllocationWidget;
class UNotificationWidget;
class UDialogueWidget;
class UQuestTrackerWidget;
class UQuestLogWidget;
class USaveSlotWidget;
enum class ESaveScreenMode : uint8;

/**
 * Main HUD class that owns and manages all in-game UMG widgets.
 * Provides show/hide API for each screen and maintains a widget stack
 * for proper input mode transitions.
 *
 * Auto-binds the HUD widget and ability bar to the player's ASC on
 * pawn possession, and refreshes ability bar when the loadout changes.
 */
UCLASS()
class POF_API AARPGHUD : public AHUD
{
	GENERATED_BODY()

public:
	AARPGHUD();

	virtual void BeginPlay() override;

	// --- Widget access ---

	UFUNCTION(BlueprintPure, Category = "HUD")
	UARPGHUDWidget* GetHUDWidget() const { return HUDWidget; }

	UFUNCTION(BlueprintPure, Category = "HUD")
	UAbilityBarWidget* GetAbilityBar() const { return AbilityBar; }

	UFUNCTION(BlueprintPure, Category = "HUD")
	UNotificationWidget* GetNotificationWidget() const { return NotificationWidget; }

	UFUNCTION(BlueprintPure, Category = "HUD")
	UDialogueWidget* GetDialogueWidget() const { return DialogueWidget; }

	UFUNCTION(BlueprintPure, Category = "HUD")
	UQuestTrackerWidget* GetQuestTracker() const { return QuestTracker; }

	/** Show the dialogue widget bound to the given dialogue component. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowDialogue(class UARPGDialogueComponent* DialogueComp);

	/** Hide the dialogue widget. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void HideDialogue();

	// --- Screen toggles ---

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleInventory();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleQuestLog();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleCharacterStats();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleAttributeAllocation();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleSkillTree();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleSaveScreen(ESaveScreenMode InMode);

	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsSaveScreenOpen() const { return bSaveScreenOpen; }

	UFUNCTION(BlueprintPure, Category = "HUD")
	USaveSlotWidget* GetSaveSlotWidget() const { return SaveSlotScreen; }

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void CloseAllScreens();

	/** Show/hide the core HUD (health bars, ability bar, etc.) */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void SetHUDVisible(bool bVisible);

	/** Display a toast notification. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ShowNotification(const FText& Message, float Duration = 3.f);

	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsInventoryOpen() const { return bInventoryOpen; }

	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsQuestLogOpen() const { return bQuestLogOpen; }

	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsCharacterStatsOpen() const { return bStatsOpen; }

	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsSkillTreeOpen() const { return bSkillTreeOpen; }

protected:
	// --- Widget classes (assign in Blueprint or defaults) ---

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Classes")
	TSubclassOf<UARPGHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Classes")
	TSubclassOf<UAbilityBarWidget> AbilityBarClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Classes")
	TSubclassOf<UInventoryScreenWidget> InventoryScreenClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Classes")
	TSubclassOf<UCharacterStatsWidget> CharacterStatsClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Classes")
	TSubclassOf<UAttributeAllocationWidget> AttributeAllocationClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Classes")
	TSubclassOf<UAbilityUnlockWidget> AbilityUnlockClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Classes")
	TSubclassOf<UNotificationWidget> NotificationWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Classes")
	TSubclassOf<UDialogueWidget> DialogueWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Classes")
	TSubclassOf<UQuestTrackerWidget> QuestTrackerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Classes")
	TSubclassOf<UQuestLogWidget> QuestLogClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Classes")
	TSubclassOf<USaveSlotWidget> SaveSlotScreenClass;

private:
	// --- Widget instances ---

	UPROPERTY()
	TObjectPtr<UARPGHUDWidget> HUDWidget;

	UPROPERTY()
	TObjectPtr<UAbilityBarWidget> AbilityBar;

	UPROPERTY()
	TObjectPtr<UInventoryScreenWidget> InventoryScreen;

	UPROPERTY()
	TObjectPtr<UCharacterStatsWidget> CharacterStats;

	UPROPERTY()
	TObjectPtr<UAttributeAllocationWidget> AttributeAllocation;

	UPROPERTY()
	TObjectPtr<UAbilityUnlockWidget> AbilityUnlockScreen;

	UPROPERTY()
	TObjectPtr<UNotificationWidget> NotificationWidget;

	UPROPERTY()
	TObjectPtr<UDialogueWidget> DialogueWidget;

	UPROPERTY()
	TObjectPtr<UQuestTrackerWidget> QuestTracker;

	UPROPERTY()
	TObjectPtr<UQuestLogWidget> QuestLog;

	UPROPERTY()
	TObjectPtr<USaveSlotWidget> SaveSlotScreen;

	bool bInventoryOpen = false;
	bool bQuestLogOpen = false;
	bool bStatsOpen = false;
	bool bAttributeOpen = false;
	bool bSkillTreeOpen = false;
	bool bSaveScreenOpen = false;

	/** Count of open overlay screens — when >0, switch to Game+UI input mode. */
	int32 OpenScreenCount = 0;

	/** Bind HUD/AbilityBar to the player's ASC and character. */
	void BindWidgetsToPlayer();

	UFUNCTION()
	void OnLoadoutChanged(int32 SlotIndex);

	void OnScreenOpened();
	void OnScreenClosed();
};

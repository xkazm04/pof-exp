#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/ARPGSaveGame.h"
#include "SaveSlotWidget.generated.h"

class UVerticalBox;
class UTextBlock;
class UButton;
class UARPGSaveSubsystem;

/** Whether the save screen is in Save or Load mode. */
UENUM(BlueprintType)
enum class ESaveScreenMode : uint8
{
	Save,
	Load
};

/**
 * Save/Load screen widget. Shows 3 manual slots + 1 auto-save slot.
 * Each slot displays metadata (character name, level, play time, timestamp).
 * Save mode: clicking a slot saves to it (with overwrite confirmation for occupied slots).
 * Load mode: clicking a slot loads from it (disabled for empty slots).
 * Delete: long-press or dedicated button deletes slot data after confirmation.
 */
UCLASS()
class POF_API USaveSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Open the save screen in the given mode. */
	UFUNCTION(BlueprintCallable, Category = "UI|Save")
	void OpenScreen(ESaveScreenMode InMode);

	/** Close and hide the save screen. */
	UFUNCTION(BlueprintCallable, Category = "UI|Save")
	void CloseScreen();

	/** Get the current screen mode. */
	UFUNCTION(BlueprintPure, Category = "UI|Save")
	ESaveScreenMode GetScreenMode() const { return ScreenMode; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// --- Widget bindings (provided by Blueprint subclass) ---

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> SlotListBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	// --- Confirmation overlay ---

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ConfirmOverlay;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ConfirmText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ConfirmYesButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ConfirmNoButton;

private:
	ESaveScreenMode ScreenMode = ESaveScreenMode::Save;

	/** Index of slot pending confirmation (-1 = none). */
	int32 PendingSlotIndex = -1;

	/** Whether pending action is delete (true) or overwrite-save (false). */
	bool bPendingDelete = false;

	/** Rebuild the slot list UI from current save data. */
	void RefreshSlotList();

	/** Format seconds into "HHh MMm" string. */
	static FString FormatPlayTime(float Seconds);

	// --- Button handlers ---

	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnConfirmYes();

	UFUNCTION()
	void OnConfirmNo();

	UFUNCTION()
	void OnSaveCompleted(const FString& SlotName, bool bSuccess);

	UFUNCTION()
	void OnLoadCompleted(const FString& SlotName, bool bSuccess);

	/** Slot button click handler — routes to save/load/confirm based on mode and state. */
	void HandleSlotAction(int32 SlotIndex, const FARPGSaveSlotInfo& Info);

	/** Slot delete button handler. */
	void HandleSlotDelete(int32 SlotIndex, const FARPGSaveSlotInfo& Info);

	/** Show confirmation overlay. */
	void ShowConfirmation(const FText& Message, int32 SlotIndex, bool bIsDelete);

	/** Hide confirmation overlay. */
	void HideConfirmation();

	/** Get save subsystem. */
	UARPGSaveSubsystem* GetSaveSubsystem() const;

	// Slot button tracking for click routing
	TMap<TObjectPtr<UButton>, int32> SlotButtonToIndex;
	TMap<TObjectPtr<UButton>, int32> DeleteButtonToIndex;

	UFUNCTION()
	void OnSlotButtonClicked();

	UFUNCTION()
	void OnDeleteButtonClicked();
};

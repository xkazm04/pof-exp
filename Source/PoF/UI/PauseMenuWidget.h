#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UWidgetSwitcher;
class USettingsMenuWidget;

/**
 * Pause menu widget. Shows Resume / Settings / Save Game / Quit buttons.
 * Contains an embedded USettingsMenuWidget for the settings screen.
 */
UCLASS()
class POF_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Called by the PlayerController when the menu should close. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnResumeRequested);

	UPROPERTY(BlueprintAssignable, Category = "UI|PauseMenu")
	FOnResumeRequested OnResumeRequested;

protected:
	virtual void NativeConstruct() override;

	// --- Main Menu Page (index 0 in the switcher) ---

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SaveGameButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	/** Feedback text shown briefly after saving. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SaveFeedbackText;

	// --- Page switcher: 0 = main menu, 1 = settings ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> PageSwitcher;

	// --- Embedded settings page (index 1 in the switcher) ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USettingsMenuWidget> SettingsPage;

private:
	UFUNCTION() void OnResumeClicked();
	UFUNCTION() void OnSettingsClicked();
	UFUNCTION() void OnSaveGameClicked();
	UFUNCTION() void OnQuitClicked();
	UFUNCTION() void OnSettingsBackPressed();

	FTimerHandle SaveFeedbackTimer;
};

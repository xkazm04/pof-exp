#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UWidgetSwitcher;
class USettingsMenuWidget;

/**
 * Title screen main menu with New Game, Continue, Settings, and Quit options.
 * The Continue button is hidden when no save file exists.
 * Contains an embedded settings page via UWidgetSwitcher.
 */
UCLASS()
class POF_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Called when the player chooses to start a new game. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNewGame);

	/** Called when the player chooses to continue from a save. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnContinueGame);

	UPROPERTY(BlueprintAssignable, Category = "UI|MainMenu")
	FOnNewGame OnNewGameRequested;

	UPROPERTY(BlueprintAssignable, Category = "UI|MainMenu")
	FOnContinueGame OnContinueGameRequested;

protected:
	virtual void NativeConstruct() override;

	// --- Main Page (index 0) ---

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NewGameButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	/** Title text displayed at the top of the menu. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	/** Optional version/build text. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VersionText;

	// --- Page switcher: 0 = main, 1 = settings ---

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> PageSwitcher;

	// --- Embedded settings page ---

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USettingsMenuWidget> SettingsPage;

	/** Title text to display. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|MainMenu")
	FText GameTitle = FText::FromString(TEXT("Pillars of Fate"));

	/** Version string. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|MainMenu")
	FText VersionString = FText::FromString(TEXT("v0.1.0 - Early Development"));

private:
	UFUNCTION() void OnNewGameClicked();
	UFUNCTION() void OnContinueClicked();
	UFUNCTION() void OnSettingsClicked();
	UFUNCTION() void OnQuitClicked();
	UFUNCTION() void OnSettingsBackPressed();

	void CheckContinueAvailability();
};

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsMenuWidget.generated.h"

class UButton;
class UTextBlock;
class USlider;
class UWidgetSwitcher;
class UComboBoxString;
class UCheckBox;
class UVerticalBox;

/**
 * Settings menu with three tabs: Graphics, Audio, Controls.
 * Reads/writes FARPGGameSettings via UARPGSaveSubsystem and applies
 * graphics via UGameUserSettings.
 */
UCLASS()
class POF_API USettingsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Call to load current settings into the UI. */
	UFUNCTION(BlueprintCallable, Category = "UI|Settings")
	void LoadSettingsIntoUI();

protected:
	virtual void NativeConstruct() override;

	// --- Tab buttons ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GraphicsTabButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AudioTabButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ControlsTabButton;

	// --- Tab switcher ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> TabSwitcher;

	// =====================================================================
	// Graphics Tab (index 0)
	// =====================================================================

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> ResolutionCombo;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> QualityCombo;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> VSyncCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> FullscreenCheckBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> FrameLimitCombo;

	// =====================================================================
	// Audio Tab (index 1)
	// =====================================================================

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> MasterVolumeSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MasterVolumeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> MusicVolumeSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MusicVolumeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> SFXVolumeSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SFXVolumeText;

	// =====================================================================
	// Controls Tab (index 2)
	// =====================================================================

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> MouseSensitivitySlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> MouseSensitivityText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> InvertYCheckBox;

	/** Container for keybinding display rows (populated dynamically). */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> KeybindingsContainer;

	// --- Apply / Back ---
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ApplyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton;

public:
	/** Called when Back is pressed — parent menu should hide this widget. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSettingsBack);

	UPROPERTY(BlueprintAssignable, Category = "UI|Settings")
	FOnSettingsBack OnBackPressed;

private:
	// Tab navigation
	UFUNCTION() void OnGraphicsTabClicked();
	UFUNCTION() void OnAudioTabClicked();
	UFUNCTION() void OnControlsTabClicked();

	// Graphics
	UFUNCTION() void OnResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() void OnVSyncChanged(bool bIsChecked);
	UFUNCTION() void OnFullscreenChanged(bool bIsChecked);
	UFUNCTION() void OnFrameLimitChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	// Audio
	UFUNCTION() void OnMasterVolumeChanged(float Value);
	UFUNCTION() void OnMusicVolumeChanged(float Value);
	UFUNCTION() void OnSFXVolumeChanged(float Value);

	// Controls
	UFUNCTION() void OnMouseSensitivityChanged(float Value);
	UFUNCTION() void OnInvertYChanged(bool bIsChecked);

	// Apply / Back
	UFUNCTION() void OnApplyClicked();
	UFUNCTION() void OnBackClicked();

	void PopulateResolutions();
	void PopulateKeybindings();
	void UpdateVolumeLabel(UTextBlock* Label, float Value);

	TArray<FIntPoint> AvailableResolutions;
};

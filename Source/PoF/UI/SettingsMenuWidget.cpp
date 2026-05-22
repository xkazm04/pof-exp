#include "UI/SettingsMenuWidget.h"
#include "Framework/ARPGSaveSubsystem.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"
#include "Components/WidgetSwitcher.h"
#include "Components/ComboBoxString.h"
#include "Components/CheckBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"

void USettingsMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// --- Tab buttons ---
	if (GraphicsTabButton) GraphicsTabButton->OnClicked.AddDynamic(this, &USettingsMenuWidget::OnGraphicsTabClicked);
	if (AudioTabButton) AudioTabButton->OnClicked.AddDynamic(this, &USettingsMenuWidget::OnAudioTabClicked);
	if (ControlsTabButton) ControlsTabButton->OnClicked.AddDynamic(this, &USettingsMenuWidget::OnControlsTabClicked);

	// --- Graphics ---
	if (ResolutionCombo) ResolutionCombo->OnSelectionChanged.AddDynamic(this, &USettingsMenuWidget::OnResolutionChanged);
	if (QualityCombo) QualityCombo->OnSelectionChanged.AddDynamic(this, &USettingsMenuWidget::OnQualityChanged);
	if (VSyncCheckBox) VSyncCheckBox->OnCheckStateChanged.AddDynamic(this, &USettingsMenuWidget::OnVSyncChanged);
	if (FullscreenCheckBox) FullscreenCheckBox->OnCheckStateChanged.AddDynamic(this, &USettingsMenuWidget::OnFullscreenChanged);
	if (FrameLimitCombo) FrameLimitCombo->OnSelectionChanged.AddDynamic(this, &USettingsMenuWidget::OnFrameLimitChanged);

	// --- Audio ---
	if (MasterVolumeSlider) MasterVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::OnMasterVolumeChanged);
	if (MusicVolumeSlider) MusicVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::OnMusicVolumeChanged);
	if (SFXVolumeSlider) SFXVolumeSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::OnSFXVolumeChanged);

	// --- Controls ---
	if (MouseSensitivitySlider)
	{
		MouseSensitivitySlider->SetMinValue(0.1f);
		MouseSensitivitySlider->SetMaxValue(5.0f);
		MouseSensitivitySlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::OnMouseSensitivityChanged);
	}
	if (InvertYCheckBox) InvertYCheckBox->OnCheckStateChanged.AddDynamic(this, &USettingsMenuWidget::OnInvertYChanged);

	// --- Apply / Back ---
	if (ApplyButton) ApplyButton->OnClicked.AddDynamic(this, &USettingsMenuWidget::OnApplyClicked);
	if (BackButton) BackButton->OnClicked.AddDynamic(this, &USettingsMenuWidget::OnBackClicked);

	// Populate combo box options
	PopulateResolutions();

	if (QualityCombo)
	{
		QualityCombo->ClearOptions();
		QualityCombo->AddOption(TEXT("Low"));
		QualityCombo->AddOption(TEXT("Medium"));
		QualityCombo->AddOption(TEXT("High"));
		QualityCombo->AddOption(TEXT("Ultra"));
	}

	if (FrameLimitCombo)
	{
		FrameLimitCombo->ClearOptions();
		FrameLimitCombo->AddOption(TEXT("Unlimited"));
		FrameLimitCombo->AddOption(TEXT("30"));
		FrameLimitCombo->AddOption(TEXT("60"));
		FrameLimitCombo->AddOption(TEXT("120"));
		FrameLimitCombo->AddOption(TEXT("144"));
	}

	PopulateKeybindings();
	LoadSettingsIntoUI();
}

// =========================================================================
// Load settings into UI
// =========================================================================

void USettingsMenuWidget::LoadSettingsIntoUI()
{
	UARPGSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UARPGSaveSubsystem>();
	if (!SaveSys) return;

	const FARPGGameSettings& Settings = SaveSys->GetSettings();

	// --- Graphics ---
	if (ResolutionCombo)
	{
		const FString ResStr = FString::Printf(TEXT("%dx%d"), Settings.Resolution.X, Settings.Resolution.Y);
		ResolutionCombo->SetSelectedOption(ResStr);
	}

	if (QualityCombo)
	{
		const TArray<FString> QualityNames = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Ultra") };
		const int32 Idx = FMath::Clamp(Settings.GraphicsQuality, 0, 3);
		QualityCombo->SetSelectedOption(QualityNames[Idx]);
	}

	if (VSyncCheckBox) VSyncCheckBox->SetIsChecked(Settings.bVSyncEnabled);
	if (FullscreenCheckBox) FullscreenCheckBox->SetIsChecked(Settings.bFullscreen);

	// Frame limit from UGameUserSettings
	if (FrameLimitCombo)
	{
		UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
		if (UserSettings)
		{
			const float Limit = UserSettings->GetFrameRateLimit();
			if (Limit <= 0.f) FrameLimitCombo->SetSelectedOption(TEXT("Unlimited"));
			else if (Limit <= 30.f) FrameLimitCombo->SetSelectedOption(TEXT("30"));
			else if (Limit <= 60.f) FrameLimitCombo->SetSelectedOption(TEXT("60"));
			else if (Limit <= 120.f) FrameLimitCombo->SetSelectedOption(TEXT("120"));
			else FrameLimitCombo->SetSelectedOption(TEXT("144"));
		}
	}

	// --- Audio ---
	if (MasterVolumeSlider) MasterVolumeSlider->SetValue(Settings.MasterVolume);
	if (MusicVolumeSlider) MusicVolumeSlider->SetValue(Settings.MusicVolume);
	if (SFXVolumeSlider) SFXVolumeSlider->SetValue(Settings.SFXVolume);

	UpdateVolumeLabel(MasterVolumeText, Settings.MasterVolume);
	UpdateVolumeLabel(MusicVolumeText, Settings.MusicVolume);
	UpdateVolumeLabel(SFXVolumeText, Settings.SFXVolume);

	// --- Controls ---
	if (MouseSensitivitySlider) MouseSensitivitySlider->SetValue(Settings.MouseSensitivity);
	if (MouseSensitivityText)
	{
		MouseSensitivityText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Settings.MouseSensitivity)));
	}
	if (InvertYCheckBox) InvertYCheckBox->SetIsChecked(Settings.bInvertY);
}

// =========================================================================
// Tab navigation
// =========================================================================

void USettingsMenuWidget::OnGraphicsTabClicked()
{
	if (TabSwitcher) TabSwitcher->SetActiveWidgetIndex(0);
}

void USettingsMenuWidget::OnAudioTabClicked()
{
	if (TabSwitcher) TabSwitcher->SetActiveWidgetIndex(1);
}

void USettingsMenuWidget::OnControlsTabClicked()
{
	if (TabSwitcher) TabSwitcher->SetActiveWidgetIndex(2);
}

// =========================================================================
// Graphics callbacks
// =========================================================================

void USettingsMenuWidget::OnResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	// Parse "1920x1080" -> FIntPoint
	FString Left, Right;
	if (SelectedItem.Split(TEXT("x"), &Left, &Right))
	{
		UARPGSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UARPGSaveSubsystem>();
		if (SaveSys)
		{
			SaveSys->GetSettings().Resolution = FIntPoint(FCString::Atoi(*Left), FCString::Atoi(*Right));
		}
	}
}

void USettingsMenuWidget::OnQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	UARPGSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UARPGSaveSubsystem>();
	if (!SaveSys) return;

	int32 Quality = 3;
	if (SelectedItem == TEXT("Low")) Quality = 0;
	else if (SelectedItem == TEXT("Medium")) Quality = 1;
	else if (SelectedItem == TEXT("High")) Quality = 2;
	else Quality = 3;

	SaveSys->GetSettings().GraphicsQuality = Quality;
}

void USettingsMenuWidget::OnVSyncChanged(bool bIsChecked)
{
	UARPGSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UARPGSaveSubsystem>();
	if (SaveSys) SaveSys->GetSettings().bVSyncEnabled = bIsChecked;
}

void USettingsMenuWidget::OnFullscreenChanged(bool bIsChecked)
{
	UARPGSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UARPGSaveSubsystem>();
	if (SaveSys) SaveSys->GetSettings().bFullscreen = bIsChecked;
}

void USettingsMenuWidget::OnFrameLimitChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!UserSettings) return;

	float Limit = 0.f; // Unlimited
	if (SelectedItem == TEXT("30")) Limit = 30.f;
	else if (SelectedItem == TEXT("60")) Limit = 60.f;
	else if (SelectedItem == TEXT("120")) Limit = 120.f;
	else if (SelectedItem == TEXT("144")) Limit = 144.f;

	UserSettings->SetFrameRateLimit(Limit);
}

// =========================================================================
// Audio callbacks
// =========================================================================

void USettingsMenuWidget::OnMasterVolumeChanged(float Value)
{
	UARPGSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UARPGSaveSubsystem>();
	if (SaveSys) SaveSys->GetSettings().MasterVolume = Value;
	UpdateVolumeLabel(MasterVolumeText, Value);
}

void USettingsMenuWidget::OnMusicVolumeChanged(float Value)
{
	UARPGSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UARPGSaveSubsystem>();
	if (SaveSys) SaveSys->GetSettings().MusicVolume = Value;
	UpdateVolumeLabel(MusicVolumeText, Value);
}

void USettingsMenuWidget::OnSFXVolumeChanged(float Value)
{
	UARPGSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UARPGSaveSubsystem>();
	if (SaveSys) SaveSys->GetSettings().SFXVolume = Value;
	UpdateVolumeLabel(SFXVolumeText, Value);
}

// =========================================================================
// Controls callbacks
// =========================================================================

void USettingsMenuWidget::OnMouseSensitivityChanged(float Value)
{
	UARPGSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UARPGSaveSubsystem>();
	if (SaveSys) SaveSys->GetSettings().MouseSensitivity = Value;
	if (MouseSensitivityText)
	{
		MouseSensitivityText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), Value)));
	}
}

void USettingsMenuWidget::OnInvertYChanged(bool bIsChecked)
{
	UARPGSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UARPGSaveSubsystem>();
	if (SaveSys) SaveSys->GetSettings().bInvertY = bIsChecked;
}

// =========================================================================
// Apply / Back
// =========================================================================

void USettingsMenuWidget::OnApplyClicked()
{
	UARPGSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UARPGSaveSubsystem>();
	if (SaveSys)
	{
		SaveSys->ApplyGraphicsSettings();
		SaveSys->SaveSettings();
	}

	// Also apply frame limit
	UGameUserSettings* UserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (UserSettings)
	{
		UserSettings->ApplySettings(false);
	}
}

void USettingsMenuWidget::OnBackClicked()
{
	OnBackPressed.Broadcast();
}

// =========================================================================
// Helpers
// =========================================================================

void USettingsMenuWidget::PopulateResolutions()
{
	if (!ResolutionCombo) return;

	ResolutionCombo->ClearOptions();
	AvailableResolutions.Empty();

	// Common resolutions
	const TArray<FIntPoint> CommonRes = {
		{1280, 720}, {1366, 768}, {1600, 900}, {1920, 1080},
		{2560, 1440}, {3840, 2160}
	};

	// Get screen resolution to filter out those larger than display
	FIntPoint ScreenRes;
	if (GEngine && GEngine->GetGameUserSettings())
	{
		ScreenRes = GEngine->GetGameUserSettings()->GetDesktopResolution();
	}
	else
	{
		ScreenRes = FIntPoint(3840, 2160);
	}

	for (const FIntPoint& Res : CommonRes)
	{
		if (Res.X <= ScreenRes.X && Res.Y <= ScreenRes.Y)
		{
			AvailableResolutions.Add(Res);
			ResolutionCombo->AddOption(FString::Printf(TEXT("%dx%d"), Res.X, Res.Y));
		}
	}
}

void USettingsMenuWidget::PopulateKeybindings()
{
	if (!KeybindingsContainer) return;

	KeybindingsContainer->ClearChildren();

	const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Regular", 13);

	// Display known keybindings as read-only rows
	struct FKeybindEntry
	{
		FString Action;
		FString Key;
	};

	const TArray<FKeybindEntry> Bindings = {
		{TEXT("Move"),              TEXT("W / A / S / D")},
		{TEXT("Primary Attack"),    TEXT("Left Mouse")},
		{TEXT("Dodge"),             TEXT("Space")},
		{TEXT("Sprint"),            TEXT("Left Shift")},
		{TEXT("Interact"),          TEXT("F")},
		{TEXT("Camera Zoom"),       TEXT("Mouse Wheel")},
		{TEXT("Camera Left/Right"), TEXT("Q / E")},
		{TEXT("Use Potion"),        TEXT("1")},
		{TEXT("Inventory"),         TEXT("I")},
		{TEXT("Character Stats"),   TEXT("C")},
		{TEXT("Pause"),             TEXT("Escape")},
	};

	for (const FKeybindEntry& Entry : Bindings)
	{
		UHorizontalBox* Row = NewObject<UHorizontalBox>(this);

		UTextBlock* ActionLabel = NewObject<UTextBlock>(this);
		ActionLabel->SetText(FText::FromString(Entry.Action));
		ActionLabel->SetFont(Font);
		ActionLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f, 1.f)));

		UHorizontalBoxSlot* ActionSlot = Row->AddChildToHorizontalBox(ActionLabel);
		ActionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ActionSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);

		UTextBlock* KeyLabel = NewObject<UTextBlock>(this);
		KeyLabel->SetText(FText::FromString(Entry.Key));
		KeyLabel->SetFont(Font);
		KeyLabel->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.85f, 0.3f, 1.f)));

		UHorizontalBoxSlot* KeySlot = Row->AddChildToHorizontalBox(KeyLabel);
		KeySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		KeySlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
		KeySlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Center);

		UVerticalBoxSlot* RowSlot = KeybindingsContainer->AddChildToVerticalBox(Row);
		RowSlot->SetPadding(FMargin(4.f, 3.f));
	}
}

void USettingsMenuWidget::UpdateVolumeLabel(UTextBlock* Label, float Value)
{
	if (Label)
	{
		Label->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f))));
	}
}

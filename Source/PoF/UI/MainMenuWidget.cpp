#include "UI/MainMenuWidget.h"
#include "UI/SettingsMenuWidget.h"
#include "Framework/ARPGSaveSubsystem.h"
#include "Framework/ARPGBuildVersion.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/KismetSystemLibrary.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Button bindings
	if (NewGameButton) NewGameButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnNewGameClicked);
	if (ContinueButton) ContinueButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnContinueClicked);
	if (SettingsButton) SettingsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnSettingsClicked);
	if (QuitButton) QuitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnQuitClicked);

	// Settings back button
	if (SettingsPage)
	{
		SettingsPage->OnBackPressed.AddDynamic(this, &UMainMenuWidget::OnSettingsBackPressed);
	}

	// Start on main page
	if (PageSwitcher) PageSwitcher->SetActiveWidgetIndex(0);

	// Title
	if (TitleText) TitleText->SetText(GameTitle);

	// Pull version from subsystem if available, fall back to configured string
	if (VersionText)
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UARPGBuildVersionSubsystem* VerSub = GI->GetSubsystem<UARPGBuildVersionSubsystem>())
			{
				VersionText->SetText(FText::FromString(VerSub->GetDisplayVersionString()));
			}
			else
			{
				VersionText->SetText(VersionString);
			}
		}
		else
		{
			VersionText->SetText(VersionString);
		}
	}

	// Show/hide Continue based on save existence
	CheckContinueAvailability();
}

// ---------------------------------------------------------------------------
// Button handlers
// ---------------------------------------------------------------------------

void UMainMenuWidget::OnNewGameClicked()
{
	OnNewGameRequested.Broadcast();
}

void UMainMenuWidget::OnContinueClicked()
{
	OnContinueGameRequested.Broadcast();
}

void UMainMenuWidget::OnSettingsClicked()
{
	if (PageSwitcher) PageSwitcher->SetActiveWidgetIndex(1);
	if (SettingsPage) SettingsPage->LoadSettingsIntoUI();
}

void UMainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, true);
}

void UMainMenuWidget::OnSettingsBackPressed()
{
	if (PageSwitcher) PageSwitcher->SetActiveWidgetIndex(0);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void UMainMenuWidget::CheckContinueAvailability()
{
	if (!ContinueButton) return;

	bool bHasSave = false;

	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		UARPGSaveSubsystem* SaveSys = GI->GetSubsystem<UARPGSaveSubsystem>();
		if (SaveSys)
		{
			bHasSave = SaveSys->DoesSaveExist(SaveSys->GetDefaultSlotName());
		}
	}

	ContinueButton->SetIsEnabled(bHasSave);
	ContinueButton->SetRenderOpacity(bHasSave ? 1.f : 0.4f);
}

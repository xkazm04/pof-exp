#include "UI/PauseMenuWidget.h"
#include "UI/SettingsMenuWidget.h"
#include "Framework/ARPGGameInstance.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResumeButton) ResumeButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnResumeClicked);
	if (SettingsButton) SettingsButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnSettingsClicked);
	if (SaveGameButton) SaveGameButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnSaveGameClicked);
	if (QuitButton) QuitButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnQuitClicked);

	// Bind settings back button to return to main page
	if (SettingsPage)
	{
		SettingsPage->OnBackPressed.AddDynamic(this, &UPauseMenuWidget::OnSettingsBackPressed);
	}

	// Start on main menu page
	if (PageSwitcher) PageSwitcher->SetActiveWidgetIndex(0);

	// Hide save feedback
	if (SaveFeedbackText) SaveFeedbackText->SetVisibility(ESlateVisibility::Hidden);
}

void UPauseMenuWidget::OnResumeClicked()
{
	OnResumeRequested.Broadcast();
}

void UPauseMenuWidget::OnSettingsClicked()
{
	if (PageSwitcher) PageSwitcher->SetActiveWidgetIndex(1);

	if (SettingsPage) SettingsPage->LoadSettingsIntoUI();
}

void UPauseMenuWidget::OnSaveGameClicked()
{
	UARPGGameInstance* GI = Cast<UARPGGameInstance>(GetGameInstance());
	if (GI)
	{
		const bool bSuccess = GI->QuickSave();

		if (SaveFeedbackText)
		{
			SaveFeedbackText->SetText(bSuccess
				? FText::FromString(TEXT("Game Saved!"))
				: FText::FromString(TEXT("Save Failed!")));
			SaveFeedbackText->SetColorAndOpacity(FSlateColor(
				bSuccess ? FLinearColor(0.2f, 1.f, 0.3f, 1.f) : FLinearColor(1.f, 0.3f, 0.2f, 1.f)));
			SaveFeedbackText->SetVisibility(ESlateVisibility::HitTestInvisible);

			// Hide after 2 seconds
			if (const UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(SaveFeedbackTimer);
				World->GetTimerManager().SetTimer(SaveFeedbackTimer, [this]()
				{
					if (SaveFeedbackText) SaveFeedbackText->SetVisibility(ESlateVisibility::Hidden);
				}, 2.0f, false);
			}
		}
	}
}

void UPauseMenuWidget::OnQuitClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	// Unpause before traveling
	UGameplayStatics::SetGamePaused(this, false);

	// Travel to main menu map — if no main menu map is configured, quit the app
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		// Use console command to go to main menu (front end) map
		// If your project has a dedicated main menu map, change this path
		PC->ConsoleCommand(TEXT("disconnect"));
	}
}

void UPauseMenuWidget::OnSettingsBackPressed()
{
	if (PageSwitcher) PageSwitcher->SetActiveWidgetIndex(0);
}

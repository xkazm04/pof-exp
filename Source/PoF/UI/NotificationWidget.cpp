#include "UI/NotificationWidget.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

void UNotificationWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UNotificationWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	for (int32 i = ActiveToasts.Num() - 1; i >= 0; --i)
	{
		FToastEntry& Toast = ActiveToasts[i];

		if (!Toast.bFading)
		{
			Toast.RemainingTime -= InDeltaTime;
			if (Toast.RemainingTime <= 0.f)
			{
				Toast.bFading = true;
				Toast.FadeTime = 0.f;
			}
		}
		else
		{
			Toast.FadeTime += InDeltaTime;
			const float Alpha = 1.f - FMath::Clamp(Toast.FadeTime / FMath::Max(FadeOutDuration, 0.01f), 0.f, 1.f);

			if (Toast.TextWidget)
			{
				Toast.TextWidget->SetRenderOpacity(Alpha);
			}

			if (Toast.FadeTime >= FadeOutDuration)
			{
				// Remove this toast
				if (Toast.TextWidget)
				{
					Toast.TextWidget->RemoveFromParent();
				}
				ActiveToasts.RemoveAt(i);
			}
		}
	}
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void UNotificationWidget::ShowToast(const FText& Message, float Duration)
{
	AddToast(Message, DefaultColor, Duration);
}

void UNotificationWidget::ShowLootPickup(const FText& ItemName, FLinearColor RarityColor, int32 Quantity)
{
	FText Message;
	if (Quantity > 1)
	{
		Message = FText::FromString(FString::Printf(TEXT("Picked up %s x%d"), *ItemName.ToString(), Quantity));
	}
	else
	{
		Message = FText::FromString(FString::Printf(TEXT("Picked up %s"), *ItemName.ToString()));
	}

	AddToast(Message, RarityColor, 3.f);
}

void UNotificationWidget::ShowQuestUpdate(const FText& QuestName, const FText& Status)
{
	FText Message = FText::FromString(FString::Printf(TEXT("[Quest] %s - %s"), *QuestName.ToString(), *Status.ToString()));
	AddToast(Message, QuestColor, 4.f);
}

void UNotificationWidget::ShowAchievement(const FText& AchievementName)
{
	FText Message = FText::FromString(FString::Printf(TEXT("Achievement Unlocked: %s"), *AchievementName.ToString()));
	AddToast(Message, AchievementColor, 5.f);
}

// ---------------------------------------------------------------------------
// Internal
// ---------------------------------------------------------------------------

void UNotificationWidget::AddToast(const FText& Message, FLinearColor Color, float Duration)
{
	if (!ToastContainer) return;

	// Enforce max visible — remove oldest
	while (ActiveToasts.Num() >= MaxVisibleToasts)
	{
		if (ActiveToasts[0].TextWidget)
		{
			ActiveToasts[0].TextWidget->RemoveFromParent();
		}
		ActiveToasts.RemoveAt(0);
	}

	// Create text widget
	UTextBlock* Text = NewObject<UTextBlock>(this);
	Text->SetText(Message);
	Text->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", ToastFontSize));
	Text->SetColorAndOpacity(FSlateColor(Color));
	Text->SetShadowOffset(FVector2D(1.f, 1.f));
	Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.8f));

	UVerticalBoxSlot* BoxSlot = ToastContainer->AddChildToVerticalBox(Text);
	if (BoxSlot)
	{
		BoxSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Right);
		BoxSlot->SetPadding(FMargin(0.f, 2.f));
	}

	FToastEntry Entry;
	Entry.TextWidget = Text;
	Entry.RemainingTime = Duration;
	Entry.FadeTime = 0.f;
	Entry.bFading = false;

	ActiveToasts.Add(Entry);
}

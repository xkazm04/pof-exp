#include "UI/LoadingScreenWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void ULoadingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LoadingProgressBar)
	{
		LoadingProgressBar->SetFillColorAndOpacity(ProgressBarColor);
		LoadingProgressBar->SetPercent(0.f);
	}

	// Default tips if none configured
	if (LoadingTips.Num() == 0)
	{
		LoadingTips.Add(FText::FromString(TEXT("Dodge at the right moment to avoid damage.")));
		LoadingTips.Add(FText::FromString(TEXT("Allocate attribute points after leveling up to grow stronger.")));
		LoadingTips.Add(FText::FromString(TEXT("Enemies with higher levels deal more damage and have more health.")));
		LoadingTips.Add(FText::FromString(TEXT("Elemental resistances cap at 90% to prevent full immunity.")));
		LoadingTips.Add(FText::FromString(TEXT("Use potions wisely — they have a cooldown between uses.")));
	}

	SetRenderOpacity(0.f);
}

void ULoadingScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	switch (FadeState)
	{
	case ELoadingFadeState::Idle:
		break;

	case ELoadingFadeState::FadingIn:
		FadeAlpha += InDeltaTime / FMath::Max(FadeInDuration, 0.01f);
		if (FadeAlpha >= 1.f)
		{
			FadeAlpha = 1.f;
			FadeState = ELoadingFadeState::Visible;
		}
		SetRenderOpacity(FadeAlpha);
		break;

	case ELoadingFadeState::Visible:
		break;

	case ELoadingFadeState::FadingOut:
		FadeAlpha -= InDeltaTime / FMath::Max(FadeOutDuration, 0.01f);
		if (FadeAlpha <= 0.f)
		{
			FadeAlpha = 0.f;
			FadeState = ELoadingFadeState::Idle;
			SetRenderOpacity(0.f);
			RemoveFromParent();
		}
		else
		{
			SetRenderOpacity(FadeAlpha);
		}
		break;
	}
}

void ULoadingScreenWidget::SetProgress(float Progress)
{
	if (LoadingProgressBar)
	{
		LoadingProgressBar->SetPercent(FMath::Clamp(Progress, 0.f, 1.f));
	}
}

void ULoadingScreenWidget::SetStatusText(const FText& InText)
{
	if (StatusText)
	{
		StatusText->SetText(InText);
	}
}

void ULoadingScreenWidget::ShowRandomTip()
{
	if (!TipText || LoadingTips.Num() == 0) return;

	const int32 Index = FMath::RandRange(0, LoadingTips.Num() - 1);
	TipText->SetText(LoadingTips[Index]);
}

void ULoadingScreenWidget::Show()
{
	FadeState = ELoadingFadeState::FadingIn;
	FadeAlpha = 0.f;
	SetRenderOpacity(0.f);
	ShowRandomTip();
}

void ULoadingScreenWidget::Hide()
{
	FadeState = ELoadingFadeState::FadingOut;
}

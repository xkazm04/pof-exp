#include "UI/EnemyHealthBarWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/CoreStyle.h"

void UEnemyHealthBarWidget::BuildTree()
{
	if (!WidgetTree || HealthBar)
	{
		return; // no tree, or already built
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName(TEXT("EnemyBarRoot")));
	WidgetTree->RootWidget = Root;
	if (!Root)
	{
		return;
	}

	const FVector2D Size(200.f, 52.f);
	const FSlateFontInfo NameFont = FCoreStyle::GetDefaultFontStyle("Bold", 13);
	const FSlateFontInfo SmallFont = FCoreStyle::GetDefaultFontStyle("Bold", 11);

	// Enemy name — top-left.
	EnemyNameText = CreateStyledTextBlock(FName(TEXT("EnemyNameText")), NameFont, FLinearColor(1.f, 0.85f, 0.3f));
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(EnemyNameText)))
	{
		S->SetAnchors(FAnchors(0.f, 0.f));
		S->SetPosition(FVector2D(0.f, 0.f));
		S->SetAutoSize(true);
	}

	// Level — top-right.
	LevelText = CreateStyledTextBlock(FName(TEXT("LevelText")), SmallFont, NormalLevelColor);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(LevelText)))
	{
		S->SetAnchors(FAnchors(1.f, 0.f));
		S->SetAlignment(FVector2D(1.f, 0.f));
		S->SetPosition(FVector2D(0.f, 0.f));
		S->SetAutoSize(true);
	}

	// Health bar — full width, below the labels.
	HealthBar = CreateStyledProgressBar(FName(TEXT("HealthBar")), BarColor);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(HealthBar)))
	{
		S->SetAnchors(FAnchors(0.f, 0.f));
		S->SetPosition(FVector2D(0.f, 24.f));
		S->SetSize(FVector2D(Size.X, 16.f));
	}

	// Health text — centred over the bar.
	HealthText = CreateStyledTextBlock(FName(TEXT("HealthText")), SmallFont, FLinearColor::White);
	HealthText->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(HealthText)))
	{
		S->SetAnchors(FAnchors(0.5f, 0.f));
		S->SetAlignment(FVector2D(0.5f, 0.f));
		S->SetPosition(FVector2D(0.f, 24.f));
		S->SetAutoSize(true);
	}
}

void UEnemyHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HealthBar)
	{
		HealthBar->SetFillColorAndOpacity(BarColor);
	}

	// Start fully hidden
	SetRenderOpacity(0.f);
}

void UEnemyHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bDead) return;

	// --- Smooth bar interpolation ---
	const float TargetPercent = CurrentMaxHealth > 0.f ? CurrentHealth / CurrentMaxHealth : 0.f;
	DisplayedPercent = FMath::FInterpTo(DisplayedPercent, TargetPercent, InDeltaTime, BarInterpSpeed);
	if (HealthBar)
	{
		HealthBar->SetPercent(DisplayedPercent);
	}

	// --- Fade state machine ---
	switch (FadeState)
	{
	case EFadeState::Hidden:
		// Nothing to do — waiting for damage
		break;

	case EFadeState::FadingIn:
		FadeAlpha += InDeltaTime / FMath::Max(FadeInDuration, 0.01f);
		if (FadeAlpha >= 1.f)
		{
			FadeAlpha = 1.f;
			FadeState = EFadeState::Visible;
		}
		SetRenderOpacity(FadeAlpha);
		break;

	case EFadeState::Visible:
		TimeSinceLastDamage += InDeltaTime;
		if (TimeSinceLastDamage >= FadeOutDelay)
		{
			FadeState = EFadeState::FadingOut;
		}
		break;

	case EFadeState::FadingOut:
		FadeAlpha -= InDeltaTime / FMath::Max(FadeOutDuration, 0.01f);
		if (FadeAlpha <= 0.f)
		{
			FadeAlpha = 0.f;
			FadeState = EFadeState::Hidden;
		}
		SetRenderOpacity(FadeAlpha);
		break;
	}
}

void UEnemyHealthBarWidget::NativeDestruct()
{
	UnbindFromAbilitySystem();
	Super::NativeDestruct();
}

void UEnemyHealthBarWidget::BindToAbilitySystem(UAbilitySystemComponent* ASC)
{
	UnbindFromAbilitySystem();

	if (!ASC) return;

	BoundASC = ASC;

	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetHealthAttribute())
		.AddUObject(this, &UEnemyHealthBarWidget::OnHealthChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &UEnemyHealthBarWidget::OnMaxHealthChanged);

	// Read current values
	bool bFound = false;
	CurrentHealth = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetHealthAttribute(), bFound);
	if (!bFound) CurrentHealth = 0.f;

	CurrentMaxHealth = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetMaxHealthAttribute(), bFound);
	if (!bFound) CurrentMaxHealth = 1.f;

	// Snap bar (no interp on init)
	DisplayedPercent = CurrentMaxHealth > 0.f ? CurrentHealth / CurrentMaxHealth : 0.f;

	UpdateHealthDisplay();
}

void UEnemyHealthBarWidget::SetEnemyInfo(const FText& InName, int32 InLevel)
{
	EnemyLevel = InLevel;

	if (EnemyNameText)
	{
		EnemyNameText->SetText(InName);
	}
	if (LevelText)
	{
		LevelText->SetText(FText::FromString(FString::Printf(TEXT("Lv %d"), InLevel)));
	}
}

void UEnemyHealthBarWidget::UpdateLevelColor(int32 PlayerLevel)
{
	if (!LevelText) return;

	const int32 LevelDiff = EnemyLevel - PlayerLevel;

	FSlateColor Color;
	if (LevelDiff >= 5)
	{
		Color = FSlateColor(DangerLevelColor);
	}
	else if (LevelDiff >= 3)
	{
		// Blend between normal and danger
		const float Alpha = (LevelDiff - 3) / 2.f;
		Color = FSlateColor(FMath::Lerp(NormalLevelColor, DangerLevelColor, Alpha));
	}
	else if (LevelDiff <= -5)
	{
		Color = FSlateColor(TrivialLevelColor);
	}
	else if (LevelDiff <= -3)
	{
		const float Alpha = (-LevelDiff - 3) / 2.f;
		Color = FSlateColor(FMath::Lerp(NormalLevelColor, TrivialLevelColor, Alpha));
	}
	else
	{
		Color = FSlateColor(NormalLevelColor);
	}

	LevelText->SetColorAndOpacity(Color);
}

void UEnemyHealthBarWidget::HideForDeath()
{
	bDead = true;
	FadeState = EFadeState::FadingOut;
	// If already hidden, just stay hidden
	if (FadeAlpha <= 0.f)
	{
		FadeAlpha = 0.f;
		FadeState = EFadeState::Hidden;
		SetRenderOpacity(0.f);
	}
}

// ---------------------------------------------------------------------------
// GAS callbacks
// ---------------------------------------------------------------------------

void UEnemyHealthBarWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	const float OldHealth = CurrentHealth;
	CurrentHealth = Data.NewValue;
	UpdateHealthDisplay();

	if (bDead) return;

	// Damage taken — trigger fade in
	if (Data.NewValue < OldHealth)
	{
		TimeSinceLastDamage = 0.f;
		if (FadeState == EFadeState::Hidden || FadeState == EFadeState::FadingOut)
		{
			FadeState = EFadeState::FadingIn;
		}
		else
		{
			// Already visible — just reset the timer
			FadeState = EFadeState::Visible;
		}
	}
}

void UEnemyHealthBarWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	CurrentMaxHealth = FMath::Max(Data.NewValue, 1.f);
	UpdateHealthDisplay();
}

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------

void UEnemyHealthBarWidget::UpdateHealthDisplay()
{
	if (HealthText)
	{
		HealthText->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f / %.0f"), FMath::Max(CurrentHealth, 0.f), CurrentMaxHealth)));
	}
}

void UEnemyHealthBarWidget::UnbindFromAbilitySystem()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC) return;

	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetHealthAttribute())
		.RemoveAll(this);
	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetMaxHealthAttribute())
		.RemoveAll(this);

	BoundASC = nullptr;
}

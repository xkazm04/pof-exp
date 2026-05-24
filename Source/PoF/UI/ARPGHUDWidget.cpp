#include "UI/ARPGHUDWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/CoreStyle.h"

void UARPGHUDWidget::BuildTree()
{
	if (!WidgetTree || HealthBar)
	{
		return; // no tree, or already built
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName(TEXT("ARPGHUDRoot")));
	WidgetTree->RootWidget = Root;
	if (!Root)
	{
		return;
	}

	// --- §4 hit vignette: full-screen, behind the bars, starts invisible ---
	HitVignette = CreateImage(FName(TEXT("HitVignette")), FLinearColor(0.8f, 0.f, 0.f, 1.f));
	HitVignette->SetRenderOpacity(0.f);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(HitVignette)))
	{
		S->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		S->SetOffsets(FMargin(0.f));
	}

	const FSlateFontInfo ValueFont = FCoreStyle::GetDefaultFontStyle("Bold", 13);
	const FSlateFontInfo LevelFont = FCoreStyle::GetDefaultFontStyle("Bold", 16);
	const float Left = 40.f;
	const float BarW = 286.f;
	float Y = 40.f;

	auto AddBar = [&](UProgressBar* Bar, float PosY, float H)
	{
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(Bar)))
		{
			S->SetAnchors(FAnchors(0.f, 0.f));
			S->SetPosition(FVector2D(Left, PosY));
			S->SetSize(FVector2D(BarW, H));
		}
	};

	auto AddOverlayText = [&](FName Name, float PosY, const FSlateFontInfo& Font) -> UTextBlock*
	{
		UTextBlock* T = CreateStyledTextBlock(Name, Font, FLinearColor::White);
		if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(T)))
		{
			S->SetAnchors(FAnchors(0.f, 0.f));
			S->SetPosition(FVector2D(Left + 6.f, PosY + 2.f));
			S->SetAutoSize(true);
		}
		return T;
	};

	// Health bar + numeric text.
	HealthBar = CreateStyledProgressBar(FName(TEXT("HealthBar")), HealthBarColor);
	AddBar(HealthBar, Y, 22.f);
	HealthText = AddOverlayText(FName(TEXT("HealthText")), Y, ValueFont);
	Y += 28.f;

	// Mana bar + numeric text.
	ManaBar = CreateStyledProgressBar(FName(TEXT("ManaBar")), ManaBarColor);
	AddBar(ManaBar, Y, 22.f);
	ManaText = AddOverlayText(FName(TEXT("ManaText")), Y, ValueFont);
	Y += 28.f;

	// Stamina bar (thinner, no numeric text).
	StaminaBar = CreateStyledProgressBar(FName(TEXT("StaminaBar")), StaminaBarColor);
	AddBar(StaminaBar, Y, 12.f);
	Y += 18.f;

	// XP bar (thin) + level label to its right.
	XPBar = CreateStyledProgressBar(FName(TEXT("XPBar")), XPBarColor);
	XPBar->SetPercent(0.f);
	AddBar(XPBar, Y, 8.f);

	LevelText = CreateStyledTextBlock(FName(TEXT("LevelText")), LevelFont, FLinearColor(1.f, 0.95f, 0.6f));
	LevelText->SetText(FText::FromString(TEXT("Lv 1")));
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(LevelText)))
	{
		S->SetAnchors(FAnchors(0.f, 0.f));
		S->SetPosition(FVector2D(Left + BarW + 10.f, 40.f));
		S->SetAutoSize(true);
	}

	// Minimap placeholder — top-right (designers fill it in later).
	MinimapPlaceholder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(TEXT("MinimapPlaceholder")));
	MinimapPlaceholder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.4f));
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(MinimapPlaceholder)))
	{
		S->SetAnchors(FAnchors(1.f, 0.f));
		S->SetAlignment(FVector2D(1.f, 0.f));
		S->SetPosition(FVector2D(-40.f, 40.f));
		S->SetSize(FVector2D(180.f, 180.f));
	}
}

void UARPGHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Set initial bar colors
	if (HealthBar)
	{
		HealthBar->SetFillColorAndOpacity(HealthBarColor);
	}
	if (ManaBar)
	{
		ManaBar->SetFillColorAndOpacity(ManaBarColor);
	}
	if (StaminaBar)
	{
		StaminaBar->SetFillColorAndOpacity(StaminaBarColor);
	}
	if (XPBar)
	{
		XPBar->SetFillColorAndOpacity(XPBarColor);
		XPBar->SetPercent(0.f);
	}
}

void UARPGHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// --- Smooth bar interpolation ---
	const float TargetHealthPercent = CurrentMaxHealth > 0.f ? CurrentHealth / CurrentMaxHealth : 0.f;
	const float TargetManaPercent = CurrentMaxMana > 0.f ? CurrentMana / CurrentMaxMana : 0.f;

	DisplayedHealthPercent = FMath::FInterpTo(DisplayedHealthPercent, TargetHealthPercent, InDeltaTime, BarInterpSpeed);
	DisplayedManaPercent = FMath::FInterpTo(DisplayedManaPercent, TargetManaPercent, InDeltaTime, BarInterpSpeed);

	if (HealthBar)
	{
		HealthBar->SetPercent(DisplayedHealthPercent);
	}
	if (ManaBar)
	{
		ManaBar->SetPercent(DisplayedManaPercent);
	}

	// --- Stamina (polled from player character, not GAS) ---
	if (AARPGPlayerCharacter* Player = BoundPlayer.Get())
	{
		const float TargetStamina = Player->GetStaminaRatio();
		DisplayedStaminaPercent = FMath::FInterpTo(DisplayedStaminaPercent, TargetStamina, InDeltaTime, BarInterpSpeed);
		if (StaminaBar)
		{
			StaminaBar->SetPercent(DisplayedStaminaPercent);
		}

		// XP bar (smooth)
		const float TargetXP = Player->GetExperiencePercent();
		DisplayedXPPercent = FMath::FInterpTo(DisplayedXPPercent, TargetXP, InDeltaTime, BarInterpSpeed);
		if (XPBar)
		{
			XPBar->SetPercent(DisplayedXPPercent);
		}
	}

	// --- Low health pulse ---
	// Drive the pulse off the interpolated (displayed) percent so the bar's
	// fill and its warning color stay visually in sync.
	if (DisplayedHealthPercent < LowHealthThreshold && DisplayedHealthPercent > 0.f)
	{
		PulseTime += InDeltaTime;
		// Sine wave oscillation between 0 and 1
		const float Alpha = (FMath::Sin(PulseTime * LowHealthPulseSpeed * 2.f * PI) + 1.f) * 0.5f;
		// Pulse between a dim and a bright red so the bar reads as a clear
		// "low health" warning rather than flashing the normal bar color.
		const FLinearColor DimLowHealthColor = LowHealthColor * 0.35f;
		FLinearColor PulsedColor = FMath::Lerp(DimLowHealthColor, LowHealthColor, Alpha);
		PulsedColor.A = 1.f;
		if (HealthBar)
		{
			HealthBar->SetFillColorAndOpacity(PulsedColor);
		}
	}
	else
	{
		PulseTime = 0.f;
		if (HealthBar)
		{
			HealthBar->SetFillColorAndOpacity(HealthBarColor);
		}
	}

	// --- §4 hit-indicator vignette: decay back to invisible (~250 ms flash) ---
	if (HitVignette)
	{
		HitFlashAlpha = FMath::FInterpTo(HitFlashAlpha, 0.f, InDeltaTime, HitFlashDecay);
		HitVignette->SetRenderOpacity(HitFlashAlpha);
	}
}

void UARPGHUDWidget::NativeDestruct()
{
	UnbindFromAbilitySystem();
	BoundPlayer = nullptr;
	Super::NativeDestruct();
}

void UARPGHUDWidget::BindToAbilitySystem(UAbilitySystemComponent* ASC)
{
	// Unbind from any previous ASC
	UnbindFromAbilitySystem();

	if (!ASC) return;

	BoundASC = ASC;

	// Bind to attribute change delegates
	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetHealthAttribute())
		.AddUObject(this, &UARPGHUDWidget::OnHealthChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &UARPGHUDWidget::OnMaxHealthChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetManaAttribute())
		.AddUObject(this, &UARPGHUDWidget::OnManaChanged);

	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetMaxManaAttribute())
		.AddUObject(this, &UARPGHUDWidget::OnMaxManaChanged);

	// Read current values so the HUD is correct immediately
	bool bFound = false;
	CurrentHealth = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetHealthAttribute(), bFound);
	if (!bFound) CurrentHealth = 0.f;

	CurrentMaxHealth = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetMaxHealthAttribute(), bFound);
	if (!bFound) CurrentMaxHealth = 1.f;

	CurrentMana = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetManaAttribute(), bFound);
	if (!bFound) CurrentMana = 0.f;

	CurrentMaxMana = ASC->GetGameplayAttributeValue(UARPGAttributeSet::GetMaxManaAttribute(), bFound);
	if (!bFound) CurrentMaxMana = 1.f;

	// Snap displayed values immediately (no interpolation on init)
	DisplayedHealthPercent = CurrentMaxHealth > 0.f ? CurrentHealth / CurrentMaxHealth : 0.f;
	DisplayedManaPercent = CurrentMaxMana > 0.f ? CurrentMana / CurrentMaxMana : 0.f;

	UpdateHealthDisplay();
	UpdateManaDisplay();
}

void UARPGHUDWidget::BindToPlayerCharacter(AARPGPlayerCharacter* Player)
{
	if (!Player) return;

	BoundPlayer = Player;

	// Bind level-up delegate for level display updates
	Player->OnPlayerLevelUp.AddDynamic(this, &UARPGHUDWidget::OnPlayerLevelUp);

	// Snap stamina and XP immediately
	DisplayedStaminaPercent = Player->GetStaminaRatio();
	DisplayedXPPercent = Player->GetExperiencePercent();

	if (StaminaBar)
	{
		StaminaBar->SetPercent(DisplayedStaminaPercent);
	}
	if (XPBar)
	{
		XPBar->SetPercent(DisplayedXPPercent);
	}

	UpdateLevelDisplay();
}

// ---------------------------------------------------------------------------
// GAS attribute change callbacks
// ---------------------------------------------------------------------------

void UARPGHUDWidget::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	// §4 hit indicator: flash the screen-edge vignette when health drops.
	if (Data.NewValue < CurrentHealth)
	{
		HitFlashAlpha = HitFlashPeak;
	}
	CurrentHealth = Data.NewValue;
	UpdateHealthDisplay();
}

void UARPGHUDWidget::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	CurrentMaxHealth = FMath::Max(Data.NewValue, 1.f);
	UpdateHealthDisplay();
}

void UARPGHUDWidget::OnManaChanged(const FOnAttributeChangeData& Data)
{
	CurrentMana = Data.NewValue;
	UpdateManaDisplay();
}

void UARPGHUDWidget::OnMaxManaChanged(const FOnAttributeChangeData& Data)
{
	CurrentMaxMana = FMath::Max(Data.NewValue, 1.f);
	UpdateManaDisplay();
}

void UARPGHUDWidget::OnPlayerLevelUp(int32 NewLevel)
{
	UpdateLevelDisplay();

	// Snap XP bar to 0 on level-up (XP was consumed)
	DisplayedXPPercent = 0.f;
	if (XPBar)
	{
		XPBar->SetPercent(0.f);
	}
}

// ---------------------------------------------------------------------------
// Display update helpers
// ---------------------------------------------------------------------------

void UARPGHUDWidget::UpdateHealthDisplay()
{
	if (HealthText)
	{
		HealthText->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f / %.0f"), CurrentHealth, CurrentMaxHealth)));
	}
	// Bar percent is driven by NativeTick interpolation — no snap here
}

void UARPGHUDWidget::UpdateManaDisplay()
{
	if (ManaText)
	{
		ManaText->SetText(FText::FromString(
			FString::Printf(TEXT("%.0f / %.0f"), CurrentMana, CurrentMaxMana)));
	}
}

void UARPGHUDWidget::UpdateLevelDisplay()
{
	if (!LevelText) return;

	if (AARPGPlayerCharacter* Player = BoundPlayer.Get())
	{
		LevelText->SetText(FText::FromString(
			FString::Printf(TEXT("Lv %d"), Player->GetPlayerLevel())));
	}
}

void UARPGHUDWidget::UnbindFromAbilitySystem()
{
	UAbilitySystemComponent* ASC = BoundASC.Get();
	if (!ASC) return;

	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetHealthAttribute())
		.RemoveAll(this);
	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetMaxHealthAttribute())
		.RemoveAll(this);
	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetManaAttribute())
		.RemoveAll(this);
	ASC->GetGameplayAttributeValueChangeDelegate(UARPGAttributeSet::GetMaxManaAttribute())
		.RemoveAll(this);

	BoundASC = nullptr;
}

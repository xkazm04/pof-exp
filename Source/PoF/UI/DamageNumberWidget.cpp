#include "UI/DamageNumberWidget.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/CoreStyle.h"
#include "Kismet/GameplayStatics.h"

void UDamageNumberWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

TSharedRef<SWidget> UDamageNumberWidget::RebuildWidget()
{
	// Pure-C++ widget: ensure a widget tree exists to build into.
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	// Only build the default tree when a Blueprint subclass hasn't supplied one.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		WidgetTree->RootWidget = RootCanvas;

		if (RootCanvas && !DamageText)
		{
			DamageText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(), TEXT("DamageText"));
			DamageText->SetJustification(ETextJustify::Center);

			if (UCanvasPanelSlot* TextSlot = RootCanvas->AddChildToCanvas(DamageText))
			{
				// Auto-size around the text, anchored/aligned to its own centre.
				TextSlot->SetAutoSize(true);
				TextSlot->SetAnchors(FAnchors(0.5f, 0.5f));
				TextSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			}
		}
	}

	return Super::RebuildWidget();
}

void UDamageNumberWidget::InitDamageNumber(
	float Amount, bool bIsCrit, bool bIsHeal, FGameplayTag DamageType, FVector WorldLocation)
{
	OriginWorldLocation = WorldLocation;
	ElapsedTime = 0.f;
	bInitialized = true;
	bCrit = bIsCrit;

	// Position by the widget's own centre so the number sits on the hit point.
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));

	// Random horizontal offset for visual variety
	RandomOffsetX = FMath::FRandRange(-HorizontalSpread, HorizontalSpread);

	if (!DamageText) return;

	// Build display string
	FString DisplayString;
	if (bIsHeal)
	{
		DisplayString = FString::Printf(TEXT("+%.0f"), Amount);
	}
	else if (bIsCrit)
	{
		DisplayString = FString::Printf(TEXT("CRIT! %.0f"), Amount);
	}
	else
	{
		DisplayString = FString::Printf(TEXT("%.0f"), Amount);
	}

	DamageText->SetText(FText::FromString(DisplayString));

	// Font size
	const int32 FontSize = bIsCrit ? CritFontSize : NormalFontSize;
	DamageText->SetFont(FCoreStyle::GetDefaultFontStyle("Bold", FontSize));

	// Color based on type
	FLinearColor Color;
	if (bIsHeal)
	{
		Color = HealColor;
	}
	else if (DamageType == ARPGGameplayTags::Damage_Fire)
	{
		Color = FireColor;
	}
	else if (DamageType == ARPGGameplayTags::Damage_Ice)
	{
		Color = IceColor;
	}
	else if (DamageType == ARPGGameplayTags::Damage_Lightning)
	{
		Color = LightningColor;
	}
	else
	{
		Color = PhysicalColor;
	}

	DamageText->SetColorAndOpacity(FSlateColor(Color));
}

void UDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bInitialized) return;

	ElapsedTime += InDeltaTime;

	if (ElapsedTime >= Lifetime)
	{
		RemoveFromParent();
		return;
	}

	const float Alpha = ElapsedTime / Lifetime; // 0 -> 1

	// Project world location to screen
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	FVector2D ScreenPos;
	bool bOnScreen = UGameplayStatics::ProjectWorldToScreen(PC, OriginWorldLocation, ScreenPos, true);

	if (!bOnScreen)
	{
		SetRenderOpacity(0.f);
		return;
	}

	// Apply float upward + horizontal spread
	const float OffsetY = -FloatDistance * Alpha; // negative = upward
	const float OffsetX = RandomOffsetX;

	SetPositionInViewport(FVector2D(
		ScreenPos.X + OffsetX,
		ScreenPos.Y + OffsetY), true);

	// Fade out: full opacity for first 40%, then fade to 0
	float Opacity;
	if (Alpha < 0.4f)
	{
		Opacity = 1.f;
	}
	else
	{
		Opacity = 1.f - ((Alpha - 0.4f) / 0.6f);
	}

	SetRenderOpacity(FMath::Max(Opacity, 0.f));

	// Crits: quick scale punch (1.5x → 1.0x over first 20% of lifetime)
	if (bCrit)
	{
		float Scale = 1.f;
		if (Alpha < 0.2f)
		{
			Scale = FMath::Lerp(1.5f, 1.f, Alpha / 0.2f);
		}
		SetRenderScale(FVector2D(Scale, Scale));
	}
}

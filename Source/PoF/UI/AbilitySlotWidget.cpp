#include "UI/AbilitySlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/CoreStyle.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"

void UAbilitySlotWidget::BuildTree()
{
	if (!WidgetTree || AbilityIcon)
	{
		return; // no tree, or already built
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName(TEXT("SlotRoot")));
	WidgetTree->RootWidget = Root;
	if (!Root)
	{
		return;
	}

	const FVector2D SlotSize(64.f, 64.f);

	// Dark frame so an empty slot is still visible.
	UImage* Frame = CreateImage(FName(TEXT("SlotFrame")), FLinearColor(0.05f, 0.05f, 0.07f, 0.85f));
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(Frame)))
	{
		S->SetAnchors(FAnchors(0.f, 0.f));
		S->SetPosition(FVector2D::ZeroVector);
		S->SetSize(SlotSize);
	}

	// Ability icon fills the slot (SetIcon swaps the brush texture).
	AbilityIcon = CreateImage(FName(TEXT("AbilityIcon")), FLinearColor(0.5f, 0.5f, 0.5f, 1.f));
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(AbilityIcon)))
	{
		S->SetAnchors(FAnchors(0.f, 0.f));
		S->SetPosition(FVector2D(2.f, 2.f));
		S->SetSize(SlotSize - FVector2D(4.f, 4.f));
	}

	// Cooldown overlay — semi-transparent black, toggled by SetCooldownPercent.
	CooldownSweep = CreateImage(FName(TEXT("CooldownSweep")), FLinearColor(0.f, 0.f, 0.f, 0.6f));
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(CooldownSweep)))
	{
		S->SetAnchors(FAnchors(0.f, 0.f));
		S->SetPosition(FVector2D(2.f, 2.f));
		S->SetSize(SlotSize - FVector2D(4.f, 4.f));
	}

	const FSlateFontInfo KeyFont = FCoreStyle::GetDefaultFontStyle("Bold", 14);
	const FSlateFontInfo SmallFont = FCoreStyle::GetDefaultFontStyle("Bold", 11);

	// Keybind label — top-left.
	KeybindLabel = CreateStyledTextBlock(FName(TEXT("KeybindLabel")), KeyFont, FLinearColor::White);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(KeybindLabel)))
	{
		S->SetAnchors(FAnchors(0.f, 0.f));
		S->SetPosition(FVector2D(4.f, 2.f));
		S->SetAutoSize(true);
	}

	// Cooldown-remaining text — centre.
	CooldownText = CreateStyledTextBlock(FName(TEXT("CooldownText")), KeyFont, FLinearColor::White);
	CooldownText->SetJustification(ETextJustify::Center);
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(CooldownText)))
	{
		S->SetAnchors(FAnchors(0.5f, 0.5f));
		S->SetAlignment(FVector2D(0.5f, 0.5f));
		S->SetPosition(FVector2D::ZeroVector);
		S->SetAutoSize(true);
	}

	// Mana cost — bottom-right.
	ManaCostText = CreateStyledTextBlock(FName(TEXT("ManaCostText")), SmallFont, FLinearColor(0.5f, 0.7f, 1.f));
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Root->AddChildToCanvas(ManaCostText)))
	{
		S->SetAnchors(FAnchors(1.f, 1.f));
		S->SetAlignment(FVector2D(1.f, 1.f));
		S->SetPosition(FVector2D(-3.f, -2.f));
		S->SetAutoSize(true);
	}
}

void UAbilitySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Create dynamic material for the sweep overlay
	if (CooldownSweep && CooldownSweepMaterial)
	{
		SweepMID = UMaterialInstanceDynamic::Create(CooldownSweepMaterial, this);
		CooldownSweep->SetBrushFromMaterial(SweepMID);
		SweepMID->SetScalarParameterValue(TEXT("Percent"), 0.f);
	}

	// Start with sweep hidden
	if (CooldownSweep)
	{
		CooldownSweep->SetRenderOpacity(0.f);
	}
	if (CooldownText)
	{
		CooldownText->SetText(FText::GetEmpty());
	}
}

void UAbilitySlotWidget::SetIcon(UTexture2D* InIcon)
{
	if (!AbilityIcon) return;

	if (InIcon)
	{
		AbilityIcon->SetBrushFromTexture(InIcon);
		AbilityIcon->SetRenderOpacity(1.f);
	}
	else
	{
		AbilityIcon->SetRenderOpacity(0.3f);
	}
}

void UAbilitySlotWidget::SetKeybindLabel(const FText& InLabel)
{
	if (KeybindLabel)
	{
		KeybindLabel->SetText(InLabel);
	}
}

void UAbilitySlotWidget::SetManaCost(float InCost)
{
	if (!ManaCostText) return;

	if (InCost > 0.f)
	{
		ManaCostText->SetText(FText::FromString(FString::Printf(TEXT("%.0f"), InCost)));
	}
	else
	{
		ManaCostText->SetText(FText::GetEmpty());
	}
}

void UAbilitySlotWidget::SetCooldownPercent(float Percent, float RemainingSeconds)
{
	const float Clamped = FMath::Clamp(Percent, 0.f, 1.f);

	if (SweepMID)
	{
		SweepMID->SetScalarParameterValue(TEXT("Percent"), Clamped);
	}

	if (CooldownSweep)
	{
		CooldownSweep->SetRenderOpacity(Clamped > 0.01f ? 1.f : 0.f);
	}

	if (CooldownText)
	{
		if (RemainingSeconds > 0.1f)
		{
			CooldownText->SetText(FText::FromString(
				FString::Printf(TEXT("%.1f"), RemainingSeconds)));
		}
		else
		{
			CooldownText->SetText(FText::GetEmpty());
		}
	}
}

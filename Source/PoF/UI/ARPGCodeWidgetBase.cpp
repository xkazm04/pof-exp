#include "UI/ARPGCodeWidgetBase.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

TSharedRef<SWidget> UARPGCodeWidgetBase::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}
	BuildTree();
	return Super::RebuildWidget();
}

FSlateBrush UARPGCodeWidgetBase::MakeSolidBrush(const FLinearColor& Colour)
{
	FSlateBrush Brush;
	Brush.TintColor = FSlateColor(Colour);
	Brush.DrawAs = ESlateBrushDrawType::Box;
	Brush.ImageSize = FVector2D(16.f, 16.f);
	return Brush;
}

FProgressBarStyle UARPGCodeWidgetBase::MakeBarStyle(const FLinearColor& Fill, const FLinearColor& Track)
{
	FProgressBarStyle Style;
	Style.BackgroundImage = MakeSolidBrush(Track);
	Style.FillImage       = MakeSolidBrush(Fill);
	Style.MarqueeImage    = MakeSolidBrush(Fill);
	Style.EnableFillAnimation = false;
	return Style;
}

UProgressBar* UARPGCodeWidgetBase::CreateStyledProgressBar(FName Name, const FLinearColor& Fill)
{
	UProgressBar* Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), Name);
	Bar->SetWidgetStyle(MakeBarStyle(Fill));
	Bar->SetFillColorAndOpacity(Fill);
	Bar->SetPercent(1.f);
	return Bar;
}

UTextBlock* UARPGCodeWidgetBase::CreateStyledTextBlock(FName Name, const FSlateFontInfo& Font, const FLinearColor& Colour)
{
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	Text->SetFont(Font);
	Text->SetColorAndOpacity(FSlateColor(Colour));
	return Text;
}

UCanvasPanelSlot* UARPGCodeWidgetBase::AnchorTopLeft(UCanvasPanel* Canvas, UWidget* Child, FVector2D Position, FVector2D Size)
{
	UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Canvas->AddChildToCanvas(Child));
	if (Slot)
	{
		Slot->SetAnchors(FAnchors(0.f, 0.f));
		Slot->SetPosition(Position);
		Slot->SetSize(Size);
	}
	return Slot;
}

UCanvasPanelSlot* UARPGCodeWidgetBase::AnchorTopCentre(UCanvasPanel* Canvas, UWidget* Child, FVector2D Position, FVector2D Size, bool bAutoSize)
{
	UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Canvas->AddChildToCanvas(Child));
	if (Slot)
	{
		Slot->SetAnchors(FAnchors(0.5f, 0.f));
		Slot->SetAlignment(FVector2D(0.5f, 0.f));
		Slot->SetPosition(Position);
		if (bAutoSize) { Slot->SetAutoSize(true); }
		else { Slot->SetSize(Size); }
	}
	return Slot;
}

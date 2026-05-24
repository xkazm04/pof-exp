#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "ARPGCodeWidgetBase.generated.h"

class UProgressBar;
class UTextBlock;
class UCanvasPanel;
class UCanvasPanelSlot;
class UWidget;
class UImage;

/**
 * Base for pure-C++ UMG widgets (no companion Widget Blueprint, no BindWidget).
 *
 * Encodes two lessons that previously had to be re-learned per widget:
 *  1. Build the widget tree in RebuildWidget() (override BuildTree()) — it runs
 *     BEFORE Super::RebuildWidget() constructs the Slate tree. NativeConstruct()
 *     runs too late: mutating WidgetTree there has no visible effect.
 *  2. An empty UProgressBar is invisible with the engine default (transparent
 *     background image). MakeBarStyle() gives an explicit dark track + bright
 *     fill so a bar is visible at any percent.
 *
 * Note: AddOnScreenDebugMessage draws above all UMG. Add slice HUD widgets at
 * DefaultHUDZOrder, and suppress debug text in tests, so vision checks aren't
 * confounded.
 */
UCLASS(Abstract)
class POF_API UARPGCodeWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Z-order for slice HUD widgets: above the main ARPG HUD layers (0-15),
	 *  below on-screen debug text (which always draws on top). */
	static constexpr int32 DefaultHUDZOrder = 30;

protected:
	/** Build the widget tree here. Override this; do NOT override RebuildWidget(). */
	virtual void BuildTree() {}

	virtual TSharedRef<SWidget> RebuildWidget() override;

	// --- Styling helpers ---
	static FSlateBrush MakeSolidBrush(const FLinearColor& Colour);

	/** Dark track + bright fill — an empty UProgressBar is otherwise invisible. */
	static FProgressBarStyle MakeBarStyle(
		const FLinearColor& Fill,
		const FLinearColor& Track = FLinearColor(0.04f, 0.04f, 0.05f, 0.85f));

	/** Construct a styled UProgressBar into WidgetTree (percent initialised to 1). */
	UProgressBar* CreateStyledProgressBar(FName Name, const FLinearColor& Fill);

	/** Construct a UTextBlock into WidgetTree with the given font + colour. */
	UTextBlock* CreateStyledTextBlock(FName Name, const FSlateFontInfo& Font,
		const FLinearColor& Colour);

	/** Add Child to Canvas anchored top-left at Position with the given Size. */
	static UCanvasPanelSlot* AnchorTopLeft(UCanvasPanel* Canvas, UWidget* Child,
		FVector2D Position, FVector2D Size);

	/** Add Child to Canvas anchored+aligned top-centre. bAutoSize ignores Size. */
	static UCanvasPanelSlot* AnchorTopCentre(UCanvasPanel* Canvas, UWidget* Child,
		FVector2D Position, FVector2D Size, bool bAutoSize = false);

	/** Add Child to Canvas anchored+aligned bottom-centre. bAutoSize ignores Size.
	 *  Position is an offset from the bottom-centre (use negative Y to lift it up). */
	static UCanvasPanelSlot* AnchorBottomCentre(UCanvasPanel* Canvas, UWidget* Child,
		FVector2D Position, FVector2D Size, bool bAutoSize = false);

	/** Construct a UImage into WidgetTree with a solid-colour brush. */
	UImage* CreateImage(FName Name, const FLinearColor& Colour);
};

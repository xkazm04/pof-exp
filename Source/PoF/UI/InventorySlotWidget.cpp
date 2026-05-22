#include "UI/InventorySlotWidget.h"
#include "UI/InventoryDragDrop.h"
#include "UI/ItemTooltipWidget.h"
#include "Inventory/ARPGItemInstance.h"
#include "Inventory/ARPGItemDefinition.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/Texture2D.h"

void UInventorySlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlotSizeBox)
	{
		SlotSizeBox->SetWidthOverride(SlotSize);
		SlotSizeBox->SetHeightOverride(SlotSize);
	}

	// Default empty state
	SetItem(nullptr);
}

void UInventorySlotWidget::SetItem(UARPGItemInstance* Item)
{
	CachedItem = Item;

	if (Item && Item->Definition)
	{
		const UARPGItemDefinition* Def = Item->Definition;

		// Icon (TSoftObjectPtr — synchronous load for UI display)
		if (ItemIcon)
		{
			UTexture2D* IconTex = Def->Icon.LoadSynchronous();
			if (IconTex)
			{
				ItemIcon->SetBrushFromTexture(IconTex);
				ItemIcon->SetRenderOpacity(1.f);
			}
			else
			{
				ItemIcon->SetRenderOpacity(0.f);
			}
		}

		// Stack count
		if (StackCountText)
		{
			if (Item->StackCount > 1)
			{
				StackCountText->SetText(FText::FromString(FString::FromInt(Item->StackCount)));
				StackCountText->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				StackCountText->SetVisibility(ESlateVisibility::Collapsed);
			}
		}

		// Rarity border
		if (RarityBorder)
		{
			RarityBorder->SetBrushColor(GetRarityBorderColor(static_cast<uint8>(Def->Rarity)));
		}
	}
	else
	{
		// Empty slot
		if (ItemIcon)
		{
			ItemIcon->SetRenderOpacity(0.f);
		}
		if (StackCountText)
		{
			StackCountText->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (RarityBorder)
		{
			RarityBorder->SetBrushColor(EmptyBorderColor);
		}
	}
}

// ---------------------------------------------------------------------------
// Hover / Tooltip
// ---------------------------------------------------------------------------

void UInventorySlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (!CachedItem || !TooltipWidgetClass) return;

	ActiveTooltip = CreateWidget<UItemTooltipWidget>(GetOwningPlayer(), TooltipWidgetClass);
	if (ActiveTooltip)
	{
		ActiveTooltip->SetItem(CachedItem);
		ActiveTooltip->AddToViewport(100);
		// Position near cursor
		ActiveTooltip->SetPositionInViewport(
			InMouseEvent.GetScreenSpacePosition() + FVector2D(16.f, 16.f), true);
	}
}

void UInventorySlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (ActiveTooltip)
	{
		ActiveTooltip->RemoveFromParent();
		ActiveTooltip = nullptr;
	}
}

// ---------------------------------------------------------------------------
// Drag and Drop
// ---------------------------------------------------------------------------

FReply UInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CachedItem)
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && CachedItem)
	{
		OnSlotRightClick.Broadcast(SlotIndex);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void UInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (!CachedItem) return;

	// Hide tooltip during drag
	if (ActiveTooltip)
	{
		ActiveTooltip->RemoveFromParent();
		ActiveTooltip = nullptr;
	}

	UInventoryDragDrop* DragOp = NewObject<UInventoryDragDrop>();
	DragOp->SourceSlotIndex = SlotIndex;
	DragOp->bFromEquipment = false;
	DragOp->Payload = CachedItem;
	DragOp->DefaultDragVisual = this;
	DragOp->Pivot = EDragPivot::CenterCenter;

	OutOperation = DragOp;
}

bool UInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UInventoryDragDrop* DragOp = Cast<UInventoryDragDrop>(InOperation);
	if (!DragOp) return false;

	if (!DragOp->bFromEquipment && DragOp->SourceSlotIndex != SlotIndex)
	{
		OnSlotDrop.Broadcast(DragOp->SourceSlotIndex, SlotIndex);
		return true;
	}

	return false;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

FLinearColor UInventorySlotWidget::GetRarityBorderColor(uint8 Rarity)
{
	switch (static_cast<EARPGItemRarity>(Rarity))
	{
	case EARPGItemRarity::Common:    return FLinearColor(0.3f, 0.3f, 0.3f);
	case EARPGItemRarity::Uncommon:  return FLinearColor(0.3f, 0.8f, 0.3f);
	case EARPGItemRarity::Rare:      return FLinearColor(0.3f, 0.5f, 1.0f);
	case EARPGItemRarity::Epic:      return FLinearColor(0.6f, 0.2f, 0.9f);
	case EARPGItemRarity::Legendary: return FLinearColor(1.0f, 0.5f, 0.0f);
	default:                         return FLinearColor(0.3f, 0.3f, 0.3f);
	}
}

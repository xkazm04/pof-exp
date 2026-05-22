#include "UI/EquipmentSlotWidget.h"
#include "UI/InventoryDragDrop.h"
#include "UI/ItemTooltipWidget.h"
#include "Inventory/ARPGItemInstance.h"
#include "Inventory/ARPGItemDefinition.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"

void UEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SlotSizeBox)
	{
		SlotSizeBox->SetWidthOverride(SlotSize);
		SlotSizeBox->SetHeightOverride(SlotSize);
	}

	if (SlotLabel)
	{
		SlotLabel->SetText(GetSlotDisplayName(EquipSlot));
	}

	SetItem(nullptr);
}

void UEquipmentSlotWidget::SetItem(UARPGItemInstance* Item)
{
	CachedItem = Item;

	if (Item && Item->Definition)
	{
		if (ItemIcon)
		{
			UTexture2D* IconTex = Item->Definition->Icon.LoadSynchronous();
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
		if (SlotLabel)
		{
			SlotLabel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		if (ItemIcon)
		{
			ItemIcon->SetRenderOpacity(0.f);
		}
		if (SlotLabel)
		{
			SlotLabel->SetText(GetSlotDisplayName(EquipSlot));
			SlotLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

// ---------------------------------------------------------------------------
// Hover / Tooltip
// ---------------------------------------------------------------------------

void UEquipmentSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (!CachedItem || !TooltipWidgetClass) return;

	ActiveTooltip = CreateWidget<UItemTooltipWidget>(GetOwningPlayer(), TooltipWidgetClass);
	if (ActiveTooltip)
	{
		ActiveTooltip->SetItem(CachedItem);
		ActiveTooltip->AddToViewport(100);
		ActiveTooltip->SetPositionInViewport(
			InMouseEvent.GetScreenSpacePosition() + FVector2D(16.f, 16.f), true);
	}
}

void UEquipmentSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (ActiveTooltip)
	{
		ActiveTooltip->RemoveFromParent();
		ActiveTooltip = nullptr;
	}
}

// ---------------------------------------------------------------------------
// Right-click to unequip
// ---------------------------------------------------------------------------

FReply UEquipmentSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && CachedItem)
	{
		OnUnequipRequested.Broadcast(EquipSlot);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

// ---------------------------------------------------------------------------
// Drag and Drop (receive only — equip items dragged from inventory)
// ---------------------------------------------------------------------------

bool UEquipmentSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UInventoryDragDrop* DragOp = Cast<UInventoryDragDrop>(InOperation);
	if (!DragOp || DragOp->bFromEquipment) return false;

	OnEquipDrop.Broadcast(EquipSlot, DragOp->SourceSlotIndex);
	return true;
}

FText UEquipmentSlotWidget::GetSlotDisplayName(EEquipmentSlot Slot)
{
	switch (Slot)
	{
	case EEquipmentSlot::Weapon:  return FText::FromString(TEXT("Weapon"));
	case EEquipmentSlot::OffHand: return FText::FromString(TEXT("Off Hand"));
	case EEquipmentSlot::Helm:    return FText::FromString(TEXT("Helm"));
	case EEquipmentSlot::Chest:   return FText::FromString(TEXT("Chest"));
	case EEquipmentSlot::Gloves:  return FText::FromString(TEXT("Gloves"));
	case EEquipmentSlot::Boots:   return FText::FromString(TEXT("Boots"));
	case EEquipmentSlot::Ring1:   return FText::FromString(TEXT("Ring"));
	case EEquipmentSlot::Ring2:   return FText::FromString(TEXT("Ring"));
	case EEquipmentSlot::Amulet:  return FText::FromString(TEXT("Amulet"));
	default:                      return FText::FromString(TEXT("Slot"));
	}
}

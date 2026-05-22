#include "UI/InventoryScreenWidget.h"
#include "UI/InventorySlotWidget.h"
#include "UI/EquipmentSlotWidget.h"
#include "UI/ItemTooltipWidget.h"
#include "Inventory/ARPGInventoryComponent.h"
#include "Inventory/ARPGItemInstance.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UInventoryScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CreateGridSlots();
	CreateEquipmentSlots();
}

void UInventoryScreenWidget::NativeDestruct()
{
	if (UARPGInventoryComponent* Inv = BoundInventory.Get())
	{
		Inv->OnInventoryChanged.RemoveDynamic(this, &UInventoryScreenWidget::OnInventoryChanged);
		Inv->OnEquipmentChanged.RemoveDynamic(this, &UInventoryScreenWidget::OnEquipmentChanged);
	}

	Super::NativeDestruct();
}

void UInventoryScreenWidget::BindToInventory(UARPGInventoryComponent* InInventory)
{
	// Unbind old
	if (UARPGInventoryComponent* OldInv = BoundInventory.Get())
	{
		OldInv->OnInventoryChanged.RemoveDynamic(this, &UInventoryScreenWidget::OnInventoryChanged);
		OldInv->OnEquipmentChanged.RemoveDynamic(this, &UInventoryScreenWidget::OnEquipmentChanged);
	}

	BoundInventory = InInventory;

	if (!InInventory) return;

	InInventory->OnInventoryChanged.AddDynamic(this, &UInventoryScreenWidget::OnInventoryChanged);
	InInventory->OnEquipmentChanged.AddDynamic(this, &UInventoryScreenWidget::OnEquipmentChanged);

	RefreshAllSlots();
	RefreshAllEquipment();
}

// ---------------------------------------------------------------------------
// Grid creation
// ---------------------------------------------------------------------------

void UInventoryScreenWidget::CreateGridSlots()
{
	if (!InventoryGrid || !SlotWidgetClass) return;

	InventoryGrid->ClearChildren();
	SlotWidgets.Empty();

	const int32 TotalSlots = GridColumns * GridRows;

	for (int32 i = 0; i < TotalSlots; ++i)
	{
		UInventorySlotWidget* SlotWidget = CreateWidget<UInventorySlotWidget>(this, SlotWidgetClass);
		if (!SlotWidget) continue;

		SlotWidget->SetSlotIndex(i);
		SlotWidget->SetTooltipWidgetClass(TooltipWidgetClass);

		const int32 Row = i / GridColumns;
		const int32 Col = i % GridColumns;
		InventoryGrid->AddChildToUniformGrid(SlotWidget, Row, Col);

		// Bind drag-drop and right-click
		SlotWidget->OnSlotDrop.AddDynamic(this, &UInventoryScreenWidget::HandleSlotDrop);
		SlotWidget->OnSlotRightClick.AddDynamic(this, &UInventoryScreenWidget::HandleSlotRightClick);

		SlotWidgets.Add(SlotWidget);
	}
}

// ---------------------------------------------------------------------------
// Equipment panel creation
// ---------------------------------------------------------------------------

void UInventoryScreenWidget::CreateEquipmentSlots()
{
	if (!EquipmentPanel || !EquipSlotWidgetClass) return;

	EquipmentPanel->ClearChildren();
	EquipWidgets.Empty();

	// Equipment slots in paper-doll order
	static const EEquipmentSlot SlotOrder[] = {
		EEquipmentSlot::Helm,
		EEquipmentSlot::Amulet,
		EEquipmentSlot::Weapon,
		EEquipmentSlot::Chest,
		EEquipmentSlot::OffHand,
		EEquipmentSlot::Gloves,
		EEquipmentSlot::Ring1,
		EEquipmentSlot::Ring2,
		EEquipmentSlot::Boots
	};

	for (EEquipmentSlot EqSlot : SlotOrder)
	{
		UEquipmentSlotWidget* EqWidget = CreateWidget<UEquipmentSlotWidget>(this, EquipSlotWidgetClass);
		if (!EqWidget) continue;

		EqWidget->SetEquipmentSlot(EqSlot);
		EqWidget->SetTooltipWidgetClass(TooltipWidgetClass);

		UVerticalBoxSlot* BoxSlot = EquipmentPanel->AddChildToVerticalBox(EqWidget);
		if (BoxSlot)
		{
			BoxSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Center);
			BoxSlot->SetPadding(FMargin(2.f));
		}

		EqWidget->OnEquipDrop.AddDynamic(this, &UInventoryScreenWidget::HandleEquipDrop);
		EqWidget->OnUnequipRequested.AddDynamic(this, &UInventoryScreenWidget::HandleUnequipRequested);

		EquipWidgets.Add(EqSlot, EqWidget);
	}
}

// ---------------------------------------------------------------------------
// Refresh
// ---------------------------------------------------------------------------

void UInventoryScreenWidget::RefreshSlot(int32 SlotIndex)
{
	if (!SlotWidgets.IsValidIndex(SlotIndex)) return;

	UARPGInventoryComponent* Inv = BoundInventory.Get();
	UARPGItemInstance* Item = Inv ? Inv->GetItemAtSlot(SlotIndex) : nullptr;
	SlotWidgets[SlotIndex]->SetItem(Item);
}

void UInventoryScreenWidget::RefreshAllSlots()
{
	for (int32 i = 0; i < SlotWidgets.Num(); ++i)
	{
		RefreshSlot(i);
	}
}

void UInventoryScreenWidget::RefreshEquipmentSlot(EEquipmentSlot EqSlot)
{
	TObjectPtr<UEquipmentSlotWidget>* Found = EquipWidgets.Find(EqSlot);
	if (!Found || !(*Found)) return;

	UARPGInventoryComponent* Inv = BoundInventory.Get();
	UARPGItemInstance* Item = Inv ? Inv->GetEquippedItem(EqSlot) : nullptr;
	(*Found)->SetItem(Item);
}

void UInventoryScreenWidget::RefreshAllEquipment()
{
	for (auto& Pair : EquipWidgets)
	{
		RefreshEquipmentSlot(Pair.Key);
	}
}

// ---------------------------------------------------------------------------
// Delegate handlers
// ---------------------------------------------------------------------------

void UInventoryScreenWidget::OnInventoryChanged(int32 SlotIndex)
{
	if (SlotIndex < 0)
	{
		RefreshAllSlots();
	}
	else
	{
		RefreshSlot(SlotIndex);
	}
}

void UInventoryScreenWidget::OnEquipmentChanged(EEquipmentSlot EquipSlotType, UARPGItemInstance* Item)
{
	RefreshEquipmentSlot(EquipSlotType);
}

void UInventoryScreenWidget::HandleSlotDrop(int32 FromIndex, int32 ToIndex)
{
	UARPGInventoryComponent* Inv = BoundInventory.Get();
	if (!Inv) return;

	Inv->MoveItem(FromIndex, ToIndex);
}

void UInventoryScreenWidget::HandleEquipDrop(EEquipmentSlot EquipSlotType, int32 FromInventoryIndex)
{
	UARPGInventoryComponent* Inv = BoundInventory.Get();
	if (!Inv) return;

	Inv->EquipItem(FromInventoryIndex, EquipSlotType);
}

void UInventoryScreenWidget::HandleSlotRightClick(int32 SlotIndex)
{
	UARPGInventoryComponent* Inv = BoundInventory.Get();
	if (!Inv) return;

	UARPGItemInstance* Item = Inv->GetItemAtSlot(SlotIndex);
	if (!Item || !Item->Definition) return;

	// If it's equipment, try to auto-equip to the first allowed slot
	if (Item->Definition->Type == EARPGItemType::Weapon ||
		Item->Definition->Type == EARPGItemType::Armor)
	{
		for (EEquipmentSlot EqSlot : Item->Definition->AllowedSlots)
		{
			if (Inv->EquipItem(SlotIndex, EqSlot))
			{
				return;
			}
		}
	}
	// If it's consumable, try to use it
	else if (Item->Definition->Type == EARPGItemType::Consumable)
	{
		Inv->UseItem(SlotIndex);
	}
}

void UInventoryScreenWidget::HandleUnequipRequested(EEquipmentSlot EquipSlotType)
{
	UARPGInventoryComponent* Inv = BoundInventory.Get();
	if (!Inv) return;

	Inv->UnequipItem(EquipSlotType);
}

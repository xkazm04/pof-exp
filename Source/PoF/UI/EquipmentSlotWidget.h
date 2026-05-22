#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/ARPGItemDefinition.h"
#include "EquipmentSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UBorder;
class USizeBox;
class UARPGItemInstance;
class UItemTooltipWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentSlotDrop, EEquipmentSlot, EquipSlotType, int32, FromInventoryIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentSlotUnequip, EEquipmentSlot, EquipSlotType);

/**
 * Single equipment slot on the paper-doll panel.
 * Shows the equipped item icon or a placeholder label.
 * Accepts drag-drops from inventory to equip items.
 */
UCLASS()
class POF_API UEquipmentSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetEquipmentSlot(EEquipmentSlot InSlot) { EquipSlot = InSlot; }
	EEquipmentSlot GetEquipmentSlot() const { return EquipSlot; }

	/** Update the displayed equipped item. Pass nullptr for empty. */
	UFUNCTION(BlueprintCallable, Category = "UI|EquipmentSlot")
	void SetItem(UARPGItemInstance* Item);

	void SetTooltipWidgetClass(TSubclassOf<UItemTooltipWidget> InClass) { TooltipWidgetClass = InClass; }

	/** Fired when an inventory item is dropped onto this slot. */
	UPROPERTY(BlueprintAssignable, Category = "UI|EquipmentSlot")
	FOnEquipmentSlotDrop OnEquipDrop;

	/** Fired when the user right-clicks to unequip this slot. */
	UPROPERTY(BlueprintAssignable, Category = "UI|EquipmentSlot")
	FOnEquipmentSlotUnequip OnUnequipRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> SlotBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SlotLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SlotSizeBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|EquipmentSlot")
	TSubclassOf<UItemTooltipWidget> TooltipWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|EquipmentSlot")
	float SlotSize = 64.f;

private:
	EEquipmentSlot EquipSlot = EEquipmentSlot::Weapon;

	UPROPERTY()
	TObjectPtr<UARPGItemInstance> CachedItem;

	UPROPERTY()
	TObjectPtr<UItemTooltipWidget> ActiveTooltip;

	static FText GetSlotDisplayName(EEquipmentSlot Slot);
};

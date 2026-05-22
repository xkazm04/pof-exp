#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventorySlotWidget.generated.h"

class UImage;
class UTextBlock;
class UBorder;
class UARPGItemInstance;
class UItemTooltipWidget;
class USizeBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSlotDrop, int32, FromIndex, int32, ToIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotRightClick, int32, SlotIndex);

/**
 * Single inventory grid slot. Shows item icon, stack count badge,
 * rarity-colored border. Supports drag-and-drop and hover tooltip.
 */
UCLASS()
class POF_API UInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Set the inventory slot index this widget represents. */
	void SetSlotIndex(int32 InIndex) { SlotIndex = InIndex; }
	int32 GetSlotIndex() const { return SlotIndex; }

	/** Update the displayed item. Pass nullptr for an empty slot. */
	UFUNCTION(BlueprintCallable, Category = "UI|InventorySlot")
	void SetItem(UARPGItemInstance* Item);

	/** Set the tooltip widget class (created on hover). */
	void SetTooltipWidgetClass(TSubclassOf<UItemTooltipWidget> InClass) { TooltipWidgetClass = InClass; }

	/** Fired when an item is dropped onto this slot. */
	UPROPERTY(BlueprintAssignable, Category = "UI|InventorySlot")
	FOnSlotDrop OnSlotDrop;

	/** Fired when an item is right-clicked (e.g. to equip or use). */
	UPROPERTY(BlueprintAssignable, Category = "UI|InventorySlot")
	FOnSlotRightClick OnSlotRightClick;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> RarityBorder;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StackCountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> SlotSizeBox;

	/** Tooltip widget class to spawn on hover. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|InventorySlot")
	TSubclassOf<UItemTooltipWidget> TooltipWidgetClass;

	/** Slot visual size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|InventorySlot")
	float SlotSize = 64.f;

	/** Default border color for empty/common slots. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|InventorySlot")
	FLinearColor EmptyBorderColor = FLinearColor(0.15f, 0.15f, 0.15f, 1.f);

private:
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<UARPGItemInstance> CachedItem;

	UPROPERTY()
	TObjectPtr<UItemTooltipWidget> ActiveTooltip;

	static FLinearColor GetRarityBorderColor(uint8 Rarity);
};

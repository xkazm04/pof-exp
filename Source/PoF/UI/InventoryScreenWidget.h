#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/ARPGItemDefinition.h"
#include "InventoryScreenWidget.generated.h"

class UUniformGridPanel;
class UVerticalBox;
class UInventorySlotWidget;
class UEquipmentSlotWidget;
class UItemTooltipWidget;
class UARPGInventoryComponent;
class UARPGItemInstance;

/**
 * Full inventory screen with equipment panel (left) and
 * inventory grid (right, 6x5). Binds to UARPGInventoryComponent
 * for data and delegates.
 */
UCLASS()
class POF_API UInventoryScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Bind to the player's inventory component. */
	UFUNCTION(BlueprintCallable, Category = "UI|Inventory")
	void BindToInventory(UARPGInventoryComponent* InInventory);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// --- Widget bindings ---

	/** Grid container for inventory slots. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> InventoryGrid;

	/** Container for equipment slot widgets (vertical list on the left). */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> EquipmentPanel;

	// --- Configuration ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Inventory")
	int32 GridColumns = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Inventory")
	int32 GridRows = 5;

	/** Widget class for individual inventory slots. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Inventory")
	TSubclassOf<UInventorySlotWidget> SlotWidgetClass;

	/** Widget class for equipment slots. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Inventory")
	TSubclassOf<UEquipmentSlotWidget> EquipSlotWidgetClass;

	/** Tooltip widget class for item hover. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Inventory")
	TSubclassOf<UItemTooltipWidget> TooltipWidgetClass;

private:
	UPROPERTY()
	TArray<TObjectPtr<UInventorySlotWidget>> SlotWidgets;

	UPROPERTY()
	TMap<EEquipmentSlot, TObjectPtr<UEquipmentSlotWidget>> EquipWidgets;

	TWeakObjectPtr<UARPGInventoryComponent> BoundInventory;

	void CreateGridSlots();
	void CreateEquipmentSlots();
	void RefreshSlot(int32 SlotIndex);
	void RefreshAllSlots();
	void RefreshEquipmentSlot(EEquipmentSlot EqSlot);
	void RefreshAllEquipment();

	UFUNCTION()
	void OnInventoryChanged(int32 SlotIndex);

	UFUNCTION()
	void OnEquipmentChanged(EEquipmentSlot EquipSlotType, UARPGItemInstance* Item);

	UFUNCTION()
	void HandleSlotDrop(int32 FromIndex, int32 ToIndex);

	UFUNCTION()
	void HandleEquipDrop(EEquipmentSlot EquipSlotType, int32 FromInventoryIndex);

	UFUNCTION()
	void HandleSlotRightClick(int32 SlotIndex);

	UFUNCTION()
	void HandleUnequipRequested(EEquipmentSlot EquipSlotType);
};

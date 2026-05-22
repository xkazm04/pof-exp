#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Inventory/ARPGItemDefinition.h"
#include "InventoryDragDrop.generated.h"

/**
 * Drag-drop payload for inventory operations.
 * Carries the source slot index and whether it came from an equipment slot.
 */
UCLASS()
class POF_API UInventoryDragDrop : public UDragDropOperation
{
	GENERATED_BODY()

public:
	/** Source inventory slot index (only valid when bFromEquipment is false). */
	UPROPERTY(BlueprintReadWrite, Category = "DragDrop")
	int32 SourceSlotIndex = INDEX_NONE;

	/** If true, the drag originated from an equipment slot rather than inventory grid. */
	UPROPERTY(BlueprintReadWrite, Category = "DragDrop")
	bool bFromEquipment = false;

	/** The equipment slot the drag originated from (only valid when bFromEquipment is true). */
	UPROPERTY(BlueprintReadWrite, Category = "DragDrop")
	EEquipmentSlot SourceEquipmentSlot = EEquipmentSlot::Weapon;
};

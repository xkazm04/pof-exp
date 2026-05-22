#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/ARPGItemDefinition.h"
#include "Inventory/ARPGItemInstance.h"
#include "ARPGInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryChanged, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChanged, EEquipmentSlot, EquipSlotType, UARPGItemInstance*, Item);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class POF_API UARPGInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGInventoryComponent();

	/** Maximum number of inventory slots. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = "1"))
	int32 MaxSlots = 20;

	/** Fired whenever the inventory contents change. SlotIndex is the affected slot (-1 for bulk operations like Sort). */
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChanged OnInventoryChanged;

	/**
	 * Add an item to the inventory. Auto-stacks onto existing compatible stacks first,
	 * then places into the first empty slot.
	 * @return The number of units that could NOT be added (0 = full success).
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(UARPGItemDefinition* Definition, int32 Count = 1);

	/** Remove Count units of the item at the given slot. Removes the instance entirely if stack hits zero.
	 *  @return true if anything was removed. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(int32 SlotIndex, int32 Count = 1);

	/** Swap the contents of two slots. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveItem(int32 FromSlot, int32 ToSlot);

	/** Find the first item instance matching the given definition, or nullptr. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UARPGItemInstance* FindItemByDefinition(UARPGItemDefinition* Definition) const;

	/** Get the item at the given slot, or nullptr if empty/out of range. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	UARPGItemInstance* GetItemAtSlot(int32 SlotIndex) const;

	/** Split a stack at SlotIndex, moving SplitCount units to the first empty slot.
	 *  @return The slot index the split portion was placed in, or INDEX_NONE on failure. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 SplitStack(int32 SlotIndex, int32 SplitCount);

	/** Merge stack at FromSlot into ToSlot (must be same definition). Leftover stays in FromSlot.
	 *  @return true if any units were merged. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MergeStacks(int32 FromSlot, int32 ToSlot);

	/** Remove Count units of the given definition from anywhere in the inventory.
	 *  @return The number of units actually removed. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 RemoveItemByDefinition(UARPGItemDefinition* Definition, int32 Count = 1);

	/** Find all item instances matching the given definition. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<UARPGItemInstance*> FindAllItemsByDefinition(UARPGItemDefinition* Definition) const;

	/** Returns true if the inventory contains at least Count units of the given definition. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasItem(UARPGItemDefinition* Definition, int32 Count = 1) const;

	/** Returns all non-null items in the inventory (not including equipped items). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TArray<UARPGItemInstance*> GetAllItems() const;

	/** Returns all currently equipped items. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TMap<EEquipmentSlot, UARPGItemInstance*> GetAllEquippedItems() const;

	/** Sort inventory: by Type ascending, then Rarity descending. Null slots pushed to end. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void Sort();

	/** Returns the number of occupied slots. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetItemCount() const;

	/** Returns the total weight of all items (inventory + equipped). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	float GetTotalWeight() const;

	/** Maximum carry weight. 0 = unlimited. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = "0.0"))
	float MaxCarryWeight = 0.f;

	/** Returns true if adding Count units of the definition would exceed MaxCarryWeight. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool WouldExceedWeight(UARPGItemDefinition* Definition, int32 Count = 1) const;

	// === Consumable Usage ===

	/** Cooldown duration (seconds) between consumable uses. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Consumable", meta = (ClampMin = "0.0"))
	float ConsumableCooldown = 1.5f;

	/**
	 * Use a consumable item at the given inventory slot.
	 * Applies OnUseEffect, decrements stack, removes if empty.
	 * Respects cooldown — fails if still on cooldown.
	 * @return true if the item was used.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Consumable")
	bool UseItem(int32 SlotIndex);

	/** Use the first consumable found in the inventory (for quick-use hotkey). */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Consumable")
	bool UseFirstConsumable();

	/** Whether the consumable cooldown is currently active. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Consumable")
	bool IsConsumableOnCooldown() const;

	// === Equipment ===

	/** Fired when an equipment slot changes. Item is the newly equipped item (nullptr on unequip). */
	UPROPERTY(BlueprintAssignable, Category = "Equipment")
	FOnEquipmentChanged OnEquipmentChanged;

	/**
	 * Equip an item from an inventory slot into an equipment slot.
	 * The item must have the target slot in its AllowedSlots.
	 * If something is already equipped there, it swaps back into the inventory slot.
	 * @return true if equip succeeded.
	 */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool EquipItem(int32 InventorySlotIndex, EEquipmentSlot Slot);

	/** Unequip the item in a slot, returning it to the first empty inventory slot.
	 *  @return true if anything was unequipped. */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool UnequipItem(EEquipmentSlot Slot);

	/** Get the item currently in an equipment slot, or nullptr. */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	UARPGItemInstance* GetEquippedItem(EEquipmentSlot Slot) const;

	/** Check whether an item definition is allowed in a given slot. */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	static bool CanEquipInSlot(const UARPGItemDefinition* Definition, EEquipmentSlot Slot);

	/** Returns true if the given equipment slot has an item in it. */
	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool IsSlotOccupied(EEquipmentSlot Slot) const;

	/** Check whether the owning character meets the level requirement for an item. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MeetsLevelRequirement(const UARPGItemDefinition* Definition) const;

	// === Save/Load ===

	/**
	 * Collect save data from all inventory and equipped items.
	 * @param OutItems      All item instance data (both inventory + equipped).
	 * @param OutEquipped   Map of UniqueId → equipment slot for equipped items.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Save")
	void GatherSaveData(TArray<FARPGItemSaveData>& OutItems, TMap<FGuid, uint8>& OutEquipped) const;

	/**
	 * Restore inventory and equipment state from save data.
	 * Requires a resolver function/map to turn FPrimaryAssetId back into UARPGItemDefinition*.
	 * @param InItems     Saved item data.
	 * @param InEquipped  Map of UniqueId → equipment slot.
	 * @param DefinitionMap  Pre-resolved map of PrimaryAssetId → loaded definition.
	 */
	void RestoreFromSaveData(
		const TArray<FARPGItemSaveData>& InItems,
		const TMap<FGuid, uint8>& InEquipped,
		const TMap<FPrimaryAssetId, UARPGItemDefinition*>& DefinitionMap);

protected:
	virtual void BeginPlay() override;

private:
	/** Sparse array — null entries represent empty slots. Always sized to MaxSlots. */
	UPROPERTY()
	TArray<TObjectPtr<UARPGItemInstance>> Items;

	bool IsValidSlot(int32 Index) const;
	int32 FindFirstEmptySlot() const;

	/** Resolve the ASC from the owning actor (must implement IAbilitySystemInterface). */
	class UAbilitySystemComponent* GetOwnerASC() const;

	/** Apply the item's OnEquipEffect and all affix effects to the owner's ASC. */
	void ApplyEquipEffect(UARPGItemInstance* Instance);

	/** Remove the item's OnEquipEffect and all affix effects from the owner's ASC. */
	void RemoveEquipEffect(UARPGItemInstance* Instance);

	/** Apply each affix's GE to the ASC with SetByCaller magnitude. */
	void ApplyAffixEffects(UARPGItemInstance* Instance);

	/** Remove all active affix GE handles from the ASC. */
	void RemoveAffixEffects(UARPGItemInstance* Instance);

	/** Currently equipped items keyed by slot. */
	UPROPERTY()
	TMap<EEquipmentSlot, TObjectPtr<UARPGItemInstance>> EquippedItems;
};

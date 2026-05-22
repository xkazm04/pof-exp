#include "Inventory/ARPGInventoryComponent.h"
#include "Inventory/ARPGItemInstance.h"
#include "Inventory/ARPGItemDefinition.h"
#include "Inventory/ARPGAffixRoller.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_PotionCooldown.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Algo/Sort.h"

UARPGInventoryComponent::UARPGInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UARPGInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	Items.SetNum(MaxSlots);
}

// ---------------------------------------------------------------------------
// AddItem
// ---------------------------------------------------------------------------
int32 UARPGInventoryComponent::AddItem(UARPGItemDefinition* Definition, int32 Count)
{
	if (!Definition || Count <= 0)
	{
		return Count;
	}

	// Weight check
	if (MaxCarryWeight > 0.f && Definition->Weight > 0.f)
	{
		const float CurrentWeight = GetTotalWeight();
		const float AddedWeight = Definition->Weight * Count;
		if (CurrentWeight + AddedWeight > MaxCarryWeight)
		{
			// Calculate how many we can fit
			const int32 MaxByWeight = FMath::FloorToInt32((MaxCarryWeight - CurrentWeight) / Definition->Weight);
			if (MaxByWeight <= 0)
			{
				return Count;
			}
			const int32 Overflow = Count - MaxByWeight;
			const int32 LeftoverFromAdd = AddItem(Definition, MaxByWeight);
			return Overflow + LeftoverFromAdd;
		}
	}

	int32 Remaining = Count;

	// Phase 1: stack onto existing compatible instances
	if (Definition->MaxStackSize > 1)
	{
		for (int32 i = 0; i < Items.Num() && Remaining > 0; ++i)
		{
			UARPGItemInstance* Existing = Items[i];
			if (!Existing || Existing->Definition != Definition)
			{
				continue;
			}

			const int32 Space = Definition->MaxStackSize - Existing->StackCount;
			if (Space <= 0)
			{
				continue;
			}

			const int32 ToAdd = FMath::Min(Remaining, Space);
			Existing->StackCount += ToAdd;
			Remaining -= ToAdd;
			OnInventoryChanged.Broadcast(i);
		}
	}

	// Phase 2: create new instances in empty slots
	while (Remaining > 0)
	{
		const int32 Slot = FindFirstEmptySlot();
		if (Slot == INDEX_NONE)
		{
			break; // inventory full
		}

		UARPGItemInstance* NewItem = NewObject<UARPGItemInstance>(GetOwner());
		NewItem->Definition = Definition;

		// Roll random affixes based on rarity if the definition has an affix pool
		if (Definition->AffixPool)
		{
			UARPGAffixRoller::RollAffixes(Definition->AffixPool, Definition->Rarity, NewItem->ItemLevel, NewItem->Affixes);
		}

		const int32 ToAdd = FMath::Min(Remaining, Definition->MaxStackSize);
		NewItem->StackCount = ToAdd;
		Remaining -= ToAdd;

		Items[Slot] = NewItem;
		OnInventoryChanged.Broadcast(Slot);
	}

	return Remaining;
}

// ---------------------------------------------------------------------------
// RemoveItem
// ---------------------------------------------------------------------------
bool UARPGInventoryComponent::RemoveItem(int32 SlotIndex, int32 Count)
{
	if (!IsValidSlot(SlotIndex) || Count <= 0)
	{
		return false;
	}

	UARPGItemInstance* Instance = Items[SlotIndex];
	if (!Instance)
	{
		return false;
	}

	Instance->StackCount -= FMath::Min(Count, Instance->StackCount);

	if (Instance->StackCount <= 0)
	{
		Items[SlotIndex] = nullptr;
	}

	OnInventoryChanged.Broadcast(SlotIndex);
	return true;
}

// ---------------------------------------------------------------------------
// MoveItem
// ---------------------------------------------------------------------------
bool UARPGInventoryComponent::MoveItem(int32 FromSlot, int32 ToSlot)
{
	if (!IsValidSlot(FromSlot) || !IsValidSlot(ToSlot) || FromSlot == ToSlot)
	{
		return false;
	}

	UARPGItemInstance* FromItem = Items[FromSlot];
	UARPGItemInstance* ToItem = Items[ToSlot];

	// Auto-merge if both slots have the same stackable definition
	if (FromItem && ToItem && FromItem->Definition && ToItem->Definition
		&& FromItem->Definition == ToItem->Definition
		&& ToItem->Definition->MaxStackSize > 1)
	{
		const int32 Space = ToItem->Definition->MaxStackSize - ToItem->StackCount;
		if (Space > 0)
		{
			const int32 Transfer = FMath::Min(FromItem->StackCount, Space);
			ToItem->StackCount += Transfer;
			FromItem->StackCount -= Transfer;
			if (FromItem->StackCount <= 0)
			{
				Items[FromSlot] = nullptr;
			}
			OnInventoryChanged.Broadcast(FromSlot);
			OnInventoryChanged.Broadcast(ToSlot);
			return true;
		}
	}

	// Otherwise just swap
	Swap(Items[FromSlot], Items[ToSlot]);

	OnInventoryChanged.Broadcast(FromSlot);
	OnInventoryChanged.Broadcast(ToSlot);
	return true;
}

// ---------------------------------------------------------------------------
// FindItemByDefinition
// ---------------------------------------------------------------------------
UARPGItemInstance* UARPGInventoryComponent::FindItemByDefinition(UARPGItemDefinition* Definition) const
{
	for (const TObjectPtr<UARPGItemInstance>& Item : Items)
	{
		if (Item && Item->Definition == Definition)
		{
			return Item;
		}
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// GetItemAtSlot
// ---------------------------------------------------------------------------
UARPGItemInstance* UARPGInventoryComponent::GetItemAtSlot(int32 SlotIndex) const
{
	if (!IsValidSlot(SlotIndex))
	{
		return nullptr;
	}
	return Items[SlotIndex];
}

// ---------------------------------------------------------------------------
// Sort
// ---------------------------------------------------------------------------
void UARPGInventoryComponent::Sort()
{
	Algo::Sort(Items, [](const TObjectPtr<UARPGItemInstance>& A, const TObjectPtr<UARPGItemInstance>& B)
	{
		// Nulls to the end
		if (!A) return false;
		if (!B) return true;

		const UARPGItemDefinition* DefA = A->Definition;
		const UARPGItemDefinition* DefB = B->Definition;
		if (!DefA) return false;
		if (!DefB) return true;

		// Primary: Type ascending
		if (DefA->Type != DefB->Type)
		{
			return DefA->Type < DefB->Type;
		}

		// Secondary: Rarity descending (legendary first)
		return DefA->Rarity > DefB->Rarity;
	});

	OnInventoryChanged.Broadcast(-1);
}

// ---------------------------------------------------------------------------
// SplitStack
// ---------------------------------------------------------------------------
int32 UARPGInventoryComponent::SplitStack(int32 SlotIndex, int32 SplitCount)
{
	if (!IsValidSlot(SlotIndex) || SplitCount <= 0)
	{
		return INDEX_NONE;
	}

	UARPGItemInstance* Source = Items[SlotIndex];
	if (!Source || !Source->Definition || Source->StackCount <= SplitCount)
	{
		return INDEX_NONE;
	}

	const int32 EmptySlot = FindFirstEmptySlot();
	if (EmptySlot == INDEX_NONE)
	{
		return INDEX_NONE;
	}

	UARPGItemInstance* SplitItem = NewObject<UARPGItemInstance>(GetOwner());
	SplitItem->Definition = Source->Definition;
	SplitItem->StackCount = SplitCount;
	SplitItem->ItemLevel = Source->ItemLevel;
	SplitItem->Affixes = Source->Affixes;
	// Clear active handles on the split copy — they belong to the source
	for (FItemAffix& Affix : SplitItem->Affixes)
	{
		Affix.ActiveHandle.Invalidate();
	}

	Source->StackCount -= SplitCount;
	Items[EmptySlot] = SplitItem;

	OnInventoryChanged.Broadcast(SlotIndex);
	OnInventoryChanged.Broadcast(EmptySlot);
	return EmptySlot;
}

// ---------------------------------------------------------------------------
// MergeStacks
// ---------------------------------------------------------------------------
bool UARPGInventoryComponent::MergeStacks(int32 FromSlot, int32 ToSlot)
{
	if (!IsValidSlot(FromSlot) || !IsValidSlot(ToSlot) || FromSlot == ToSlot)
	{
		return false;
	}

	UARPGItemInstance* FromItem = Items[FromSlot];
	UARPGItemInstance* ToItem = Items[ToSlot];

	if (!FromItem || !ToItem || !FromItem->Definition || !ToItem->Definition)
	{
		return false;
	}

	if (FromItem->Definition != ToItem->Definition || ToItem->Definition->MaxStackSize <= 1)
	{
		return false;
	}

	const int32 Space = ToItem->Definition->MaxStackSize - ToItem->StackCount;
	if (Space <= 0)
	{
		return false;
	}

	const int32 Transfer = FMath::Min(FromItem->StackCount, Space);
	ToItem->StackCount += Transfer;
	FromItem->StackCount -= Transfer;

	if (FromItem->StackCount <= 0)
	{
		Items[FromSlot] = nullptr;
	}

	OnInventoryChanged.Broadcast(FromSlot);
	OnInventoryChanged.Broadcast(ToSlot);
	return true;
}

// ---------------------------------------------------------------------------
// RemoveItemByDefinition
// ---------------------------------------------------------------------------
int32 UARPGInventoryComponent::RemoveItemByDefinition(UARPGItemDefinition* Definition, int32 Count)
{
	if (!Definition || Count <= 0)
	{
		return 0;
	}

	int32 Remaining = Count;

	for (int32 i = 0; i < Items.Num() && Remaining > 0; ++i)
	{
		UARPGItemInstance* Instance = Items[i];
		if (!Instance || Instance->Definition != Definition)
		{
			continue;
		}

		const int32 ToRemove = FMath::Min(Remaining, Instance->StackCount);
		Instance->StackCount -= ToRemove;
		Remaining -= ToRemove;

		if (Instance->StackCount <= 0)
		{
			Items[i] = nullptr;
		}

		OnInventoryChanged.Broadcast(i);
	}

	return Count - Remaining;
}

// ---------------------------------------------------------------------------
// FindAllItemsByDefinition
// ---------------------------------------------------------------------------
TArray<UARPGItemInstance*> UARPGInventoryComponent::FindAllItemsByDefinition(UARPGItemDefinition* Definition) const
{
	TArray<UARPGItemInstance*> Results;
	if (!Definition)
	{
		return Results;
	}

	for (const TObjectPtr<UARPGItemInstance>& Item : Items)
	{
		if (Item && Item->Definition == Definition)
		{
			Results.Add(Item);
		}
	}
	return Results;
}

// ---------------------------------------------------------------------------
// HasItem
// ---------------------------------------------------------------------------
bool UARPGInventoryComponent::HasItem(UARPGItemDefinition* Definition, int32 Count) const
{
	if (!Definition || Count <= 0)
	{
		return Count <= 0;
	}

	int32 Total = 0;
	for (const TObjectPtr<UARPGItemInstance>& Item : Items)
	{
		if (Item && Item->Definition == Definition)
		{
			Total += Item->StackCount;
			if (Total >= Count)
			{
				return true;
			}
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
// GetAllItems
// ---------------------------------------------------------------------------
TArray<UARPGItemInstance*> UARPGInventoryComponent::GetAllItems() const
{
	TArray<UARPGItemInstance*> Results;
	for (const TObjectPtr<UARPGItemInstance>& Item : Items)
	{
		if (Item)
		{
			Results.Add(Item);
		}
	}
	return Results;
}

// ---------------------------------------------------------------------------
// GetAllEquippedItems
// ---------------------------------------------------------------------------
TMap<EEquipmentSlot, UARPGItemInstance*> UARPGInventoryComponent::GetAllEquippedItems() const
{
	TMap<EEquipmentSlot, UARPGItemInstance*> Result;
	for (const auto& Pair : EquippedItems)
	{
		if (Pair.Value)
		{
			Result.Add(Pair.Key, Pair.Value);
		}
	}
	return Result;
}

// ---------------------------------------------------------------------------
// GetItemCount
// ---------------------------------------------------------------------------
int32 UARPGInventoryComponent::GetItemCount() const
{
	int32 Count = 0;
	for (const TObjectPtr<UARPGItemInstance>& Item : Items)
	{
		if (Item)
		{
			++Count;
		}
	}
	return Count;
}

// ---------------------------------------------------------------------------
// GetTotalWeight
// ---------------------------------------------------------------------------
float UARPGInventoryComponent::GetTotalWeight() const
{
	float Total = 0.f;

	// Inventory items
	for (const TObjectPtr<UARPGItemInstance>& Item : Items)
	{
		if (Item && Item->Definition)
		{
			Total += Item->Definition->Weight * Item->StackCount;
		}
	}

	// Equipped items
	for (const auto& Pair : EquippedItems)
	{
		if (Pair.Value && Pair.Value->Definition)
		{
			Total += Pair.Value->Definition->Weight * Pair.Value->StackCount;
		}
	}

	return Total;
}

// ---------------------------------------------------------------------------
// WouldExceedWeight
// ---------------------------------------------------------------------------
bool UARPGInventoryComponent::WouldExceedWeight(UARPGItemDefinition* Definition, int32 Count) const
{
	if (MaxCarryWeight <= 0.f || !Definition || Definition->Weight <= 0.f)
	{
		return false;
	}

	return GetTotalWeight() + (Definition->Weight * Count) > MaxCarryWeight;
}

// ---------------------------------------------------------------------------
// UseItem
// ---------------------------------------------------------------------------
bool UARPGInventoryComponent::UseItem(int32 SlotIndex)
{
	if (!IsValidSlot(SlotIndex))
	{
		return false;
	}

	UARPGItemInstance* Instance = Items[SlotIndex];
	if (!Instance || !Instance->Definition)
	{
		return false;
	}

	// Must be a consumable with an OnUseEffect
	if (Instance->Definition->Type != EARPGItemType::Consumable || !Instance->Definition->OnUseEffect)
	{
		return false;
	}

	// Check level requirement
	if (!MeetsLevelRequirement(Instance->Definition))
	{
		return false;
	}

	// Check cooldown via gameplay tag on the ASC
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return false;
	}

	if (ASC->HasMatchingGameplayTag(ARPGGameplayTags::Cooldown_Potion))
	{
		return false; // still on cooldown
	}

	// Apply the use effect (e.g., instant heal)
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(Instance);

	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
		Instance->Definition->OnUseEffect, 1.f, Context);

	if (!Spec.IsValid())
	{
		return false;
	}

	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

	// Apply cooldown via GE_PotionCooldown (grants Cooldown.Potion tag for duration)
	if (ConsumableCooldown > 0.f)
	{
		const FGameplayEffectSpecHandle CdSpec = ASC->MakeOutgoingSpec(
			UGE_PotionCooldown::StaticClass(), 1.f, ASC->MakeEffectContext());

		if (CdSpec.IsValid())
		{
			CdSpec.Data->SetDuration(ConsumableCooldown, true);
			ASC->ApplyGameplayEffectSpecToSelf(*CdSpec.Data.Get());
		}
	}

	// Decrement stack; remove if empty
	Instance->StackCount -= 1;
	if (Instance->StackCount <= 0)
	{
		Items[SlotIndex] = nullptr;
	}

	OnInventoryChanged.Broadcast(SlotIndex);
	return true;
}

// ---------------------------------------------------------------------------
// UseFirstConsumable
// ---------------------------------------------------------------------------
bool UARPGInventoryComponent::UseFirstConsumable()
{
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (Items[i] && Items[i]->Definition && Items[i]->Definition->Type == EARPGItemType::Consumable)
		{
			return UseItem(i);
		}
	}
	return false;
}

// ---------------------------------------------------------------------------
// IsConsumableOnCooldown
// ---------------------------------------------------------------------------
bool UARPGInventoryComponent::IsConsumableOnCooldown() const
{
	if (const UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		return ASC->HasMatchingGameplayTag(ARPGGameplayTags::Cooldown_Potion);
	}
	return false;
}

// ---------------------------------------------------------------------------
// EquipItem
// ---------------------------------------------------------------------------
bool UARPGInventoryComponent::EquipItem(int32 InventorySlotIndex, EEquipmentSlot Slot)
{
	if (!IsValidSlot(InventorySlotIndex))
	{
		return false;
	}

	UARPGItemInstance* ItemToEquip = Items[InventorySlotIndex];
	if (!ItemToEquip || !ItemToEquip->Definition)
	{
		return false;
	}

	if (!CanEquipInSlot(ItemToEquip->Definition, Slot))
	{
		return false;
	}

	// Check level requirement
	if (!MeetsLevelRequirement(ItemToEquip->Definition))
	{
		return false;
	}

	// Remove the effect from whatever is currently equipped in this slot
	UARPGItemInstance* PreviouslyEquipped = nullptr;
	if (TObjectPtr<UARPGItemInstance>* Found = EquippedItems.Find(Slot))
	{
		PreviouslyEquipped = *Found;
		RemoveEquipEffect(PreviouslyEquipped);
	}

	// Swap: put the old item back into the inventory slot
	EquippedItems.Add(Slot, ItemToEquip);
	Items[InventorySlotIndex] = PreviouslyEquipped; // nullptr if nothing was there

	// Apply the new item's equip effect
	ApplyEquipEffect(ItemToEquip);

	OnInventoryChanged.Broadcast(InventorySlotIndex);
	OnEquipmentChanged.Broadcast(Slot, ItemToEquip);
	return true;
}

// ---------------------------------------------------------------------------
// UnequipItem
// ---------------------------------------------------------------------------
bool UARPGInventoryComponent::UnequipItem(EEquipmentSlot Slot)
{
	TObjectPtr<UARPGItemInstance>* Found = EquippedItems.Find(Slot);
	if (!Found || !(*Found))
	{
		return false;
	}

	const int32 EmptySlot = FindFirstEmptySlot();
	if (EmptySlot == INDEX_NONE)
	{
		return false; // no room in inventory
	}

	RemoveEquipEffect(*Found);

	Items[EmptySlot] = *Found;
	EquippedItems.Remove(Slot);

	OnInventoryChanged.Broadcast(EmptySlot);
	OnEquipmentChanged.Broadcast(Slot, nullptr);
	return true;
}

// ---------------------------------------------------------------------------
// GetEquippedItem
// ---------------------------------------------------------------------------
UARPGItemInstance* UARPGInventoryComponent::GetEquippedItem(EEquipmentSlot Slot) const
{
	const TObjectPtr<UARPGItemInstance>* Found = EquippedItems.Find(Slot);
	return Found ? *Found : nullptr;
}

// ---------------------------------------------------------------------------
// CanEquipInSlot
// ---------------------------------------------------------------------------
bool UARPGInventoryComponent::CanEquipInSlot(const UARPGItemDefinition* Definition, EEquipmentSlot Slot)
{
	if (!Definition)
	{
		return false;
	}
	return Definition->AllowedSlots.Contains(Slot);
}

// ---------------------------------------------------------------------------
// GAS Effect Helpers
// ---------------------------------------------------------------------------
UAbilitySystemComponent* UARPGInventoryComponent::GetOwnerASC() const
{
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return ASI->GetAbilitySystemComponent();
	}
	return nullptr;
}

void UARPGInventoryComponent::ApplyEquipEffect(UARPGItemInstance* Instance)
{
	if (!Instance || !Instance->Definition)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	// Apply the base equip effect from the item definition
	if (Instance->Definition->OnEquipEffect)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(Instance);

		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(
			Instance->Definition->OnEquipEffect, 1.f, Context);

		if (Spec.IsValid())
		{
			Instance->EquipEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	// Apply per-affix GEs
	ApplyAffixEffects(Instance);
}

void UARPGInventoryComponent::RemoveEquipEffect(UARPGItemInstance* Instance)
{
	if (!Instance)
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetOwnerASC())
	{
		if (Instance->EquipEffectHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Instance->EquipEffectHandle);
			Instance->EquipEffectHandle.Invalidate();
		}
	}

	// Remove per-affix GEs
	RemoveAffixEffects(Instance);
}

void UARPGInventoryComponent::ApplyAffixEffects(UARPGItemInstance* Instance)
{
	if (!Instance)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	for (FItemAffix& Affix : Instance->Affixes)
	{
		if (!Affix.Effect)
		{
			continue;
		}

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(Instance);

		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Affix.Effect, 1.f, Context);
		if (Spec.IsValid())
		{
			// Set the magnitude via SetByCaller tag so the GE can read it
			Spec.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_Affix_Magnitude, Affix.Magnitude);
			Affix.ActiveHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

void UARPGInventoryComponent::RemoveAffixEffects(UARPGItemInstance* Instance)
{
	if (!Instance)
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return;
	}

	for (FItemAffix& Affix : Instance->Affixes)
	{
		if (Affix.ActiveHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Affix.ActiveHandle);
			Affix.ActiveHandle.Invalidate();
		}
	}
}

// ---------------------------------------------------------------------------
// Save / Load
// ---------------------------------------------------------------------------
void UARPGInventoryComponent::GatherSaveData(TArray<FARPGItemSaveData>& OutItems, TMap<FGuid, uint8>& OutEquipped) const
{
	OutItems.Reset();
	OutEquipped.Reset();

	// Inventory items
	for (const TObjectPtr<UARPGItemInstance>& Item : Items)
	{
		if (Item)
		{
			OutItems.Add(Item->ToSaveData());
		}
	}

	// Equipped items
	for (const auto& Pair : EquippedItems)
	{
		if (Pair.Value)
		{
			OutItems.Add(Pair.Value->ToSaveData());
			OutEquipped.Add(Pair.Value->UniqueId, static_cast<uint8>(Pair.Key));
		}
	}
}

void UARPGInventoryComponent::RestoreFromSaveData(
	const TArray<FARPGItemSaveData>& InItems,
	const TMap<FGuid, uint8>& InEquipped,
	const TMap<FPrimaryAssetId, UARPGItemDefinition*>& DefinitionMap)
{
	// Clear existing state — remove any active effects first
	for (auto& Pair : EquippedItems)
	{
		if (Pair.Value)
		{
			RemoveEquipEffect(Pair.Value);
		}
	}
	EquippedItems.Reset();
	Items.SetNum(MaxSlots);
	for (auto& ItemSlot : Items)
	{
		ItemSlot = nullptr;
	}

	int32 NextSlot = 0;

	for (const FARPGItemSaveData& SaveData : InItems)
	{
		UARPGItemDefinition* const* FoundDef = DefinitionMap.Find(SaveData.DefinitionId);
		if (!FoundDef || !(*FoundDef))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Inventory] Could not resolve definition '%s' during load — skipping item"),
				*SaveData.DefinitionId.ToString());
			continue;
		}

		UARPGItemInstance* Instance = NewObject<UARPGItemInstance>(GetOwner());
		Instance->FromSaveData(SaveData, *FoundDef);

		// Check if this item was equipped
		if (const uint8* EquipSlotPtr = InEquipped.Find(SaveData.UniqueId))
		{
			const EEquipmentSlot EqSlot = static_cast<EEquipmentSlot>(*EquipSlotPtr);
			EquippedItems.Add(EqSlot, Instance);
			ApplyEquipEffect(Instance);
			OnEquipmentChanged.Broadcast(EqSlot, Instance);
		}
		else
		{
			// Place in next available inventory slot
			while (NextSlot < Items.Num() && Items[NextSlot])
			{
				++NextSlot;
			}
			if (NextSlot < Items.Num())
			{
				Items[NextSlot] = Instance;
				++NextSlot;
			}
		}
	}

	OnInventoryChanged.Broadcast(-1);
}

// ---------------------------------------------------------------------------
// IsSlotOccupied
// ---------------------------------------------------------------------------
bool UARPGInventoryComponent::IsSlotOccupied(EEquipmentSlot Slot) const
{
	const TObjectPtr<UARPGItemInstance>* Found = EquippedItems.Find(Slot);
	return Found && *Found != nullptr;
}

// ---------------------------------------------------------------------------
// MeetsLevelRequirement
// ---------------------------------------------------------------------------
bool UARPGInventoryComponent::MeetsLevelRequirement(const UARPGItemDefinition* Definition) const
{
	if (!Definition || Definition->RequiredLevel <= 0)
	{
		return true;
	}

	if (const AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(GetOwner()))
	{
		return Player->GetPlayerLevel() >= Definition->RequiredLevel;
	}

	// Non-player owners (e.g. NPCs) are not level-gated
	return true;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
bool UARPGInventoryComponent::IsValidSlot(int32 Index) const
{
	return Index >= 0 && Index < Items.Num();
}

int32 UARPGInventoryComponent::FindFirstEmptySlot() const
{
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (!Items[i])
		{
			return i;
		}
	}
	return INDEX_NONE;
}

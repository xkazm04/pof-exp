#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "ARPGItemDefinition.generated.h"

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	Weapon,
	OffHand,
	Helm,
	Chest,
	Gloves,
	Boots,
	Ring1,
	Ring2,
	Amulet
};

UENUM(BlueprintType)
enum class EARPGItemType : uint8
{
	Weapon,
	Armor,
	Consumable,
	Material,
	Quest
};

UENUM(BlueprintType)
enum class EARPGItemRarity : uint8
{
	Common,
	Uncommon,
	Rare,
	Epic,
	Legendary
};

class UGameplayEffect;
class UStaticMesh;
class UTexture2D;

UCLASS(BlueprintType)
class POF_API UARPGItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EARPGItemType Type = EARPGItemType::Material;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EARPGItemRarity Rarity = EARPGItemRarity::Common;

	/** Maximum stack size. 1 = non-stackable (e.g. weapons, armor). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

	/** Base gold/currency value of this item. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0.0"))
	float BaseValue = 0.f;

	/** Gameplay tags associated with this item (e.g. Item.Weapon.Sword, Item.Consumable.Potion). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FGameplayTagContainer ItemTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TSubclassOf<UGameplayEffect> OnEquipEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TSubclassOf<UGameplayEffect> OnUseEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
	TObjectPtr<UStaticMesh> WorldMesh;

	/**
	 * Data Table of possible affixes (row struct: FAffixTableRow).
	 * When this item is generated, random affixes are rolled from this pool
	 * based on the item's rarity tier.
	 * Leave null for items that should never get random affixes (consumables, materials, etc.).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Affixes", meta = (RowType = "/Script/PoF.AffixTableRow"))
	TObjectPtr<UDataTable> AffixPool;

	/** Weight per unit (for encumbrance). 0 = weightless. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0.0"))
	float Weight = 0.f;

	/** Minimum character level required to equip/use this item. 0 = no requirement. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0"))
	int32 RequiredLevel = 0;

	/** Which equipment slots this item may occupy. Empty = not equippable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	TArray<EEquipmentSlot> AllowedSlots;

	/** Get the UI color associated with a rarity tier. */
	UFUNCTION(BlueprintCallable, Category = "Item")
	static FLinearColor GetRarityColor(EARPGItemRarity InRarity);

	/** Get the display name for a rarity tier. */
	UFUNCTION(BlueprintCallable, Category = "Item")
	static FText GetRarityDisplayName(EARPGItemRarity InRarity);

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};

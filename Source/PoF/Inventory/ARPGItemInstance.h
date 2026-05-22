#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Inventory/ARPGItemDefinition.h"
#include "ARPGItemInstance.generated.h"
class UGameplayEffect;

// =========================================================================
// A single random affix rolled on an item instance
// =========================================================================

USTRUCT(BlueprintType)
struct FItemAffix
{
	GENERATED_BODY()

	/** Identifying tag (e.g. Affix.Strength). Matches the AffixTag from the affix pool Data Table. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix", SaveGame)
	FGameplayTag AffixTag;

	/** The gameplay effect applied by this affix. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix", SaveGame)
	TSubclassOf<UGameplayEffect> Effect;

	/** Scalar magnitude for the effect (e.g. +12 Strength). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix", SaveGame)
	float Magnitude = 0.f;

	/** Display text for this affix (e.g. "of Strength", "Blazing"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix", SaveGame)
	FText DisplayName;

	/** True if this is a prefix, false if suffix. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix", SaveGame)
	bool bIsPrefix = false;

	/** Handle for the active GE applied by this affix while equipped. Not saved. */
	FActiveGameplayEffectHandle ActiveHandle;
};

// =========================================================================
// Serializable snapshot of an item instance for save/load
// =========================================================================

USTRUCT(BlueprintType)
struct FARPGItemSaveData
{
	GENERATED_BODY()

	/** Primary asset ID of the UARPGItemDefinition (e.g. "ARPGItemDefinition:DA_IronSword"). */
	UPROPERTY(SaveGame)
	FPrimaryAssetId DefinitionId;

	/** Current stack count. */
	UPROPERTY(SaveGame)
	int32 StackCount = 1;

	/** Item level. */
	UPROPERTY(SaveGame)
	int32 ItemLevel = 1;

	/** Unique identifier for this instance. */
	UPROPERTY(SaveGame)
	FGuid UniqueId;

	/** Rolled affixes. */
	UPROPERTY(SaveGame)
	TArray<FItemAffix> Affixes;

	/** Rarity rolled at generation time. */
	UPROPERTY(SaveGame)
	EARPGItemRarity RolledRarity = EARPGItemRarity::Common;

	UPROPERTY(SaveGame)
	bool bHasRolledRarity = false;
};

// =========================================================================
// Runtime item instance — a specific item in an inventory or the world
// =========================================================================

UCLASS(BlueprintType)
class POF_API UARPGItemInstance : public UObject
{
	GENERATED_BODY()

public:
	UARPGItemInstance();

	// --- Replication ---

	/** Allows this UObject to participate in networking when owned by a replicated Actor/Component. */
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** The data asset that defines this item's base properties. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", SaveGame, Replicated)
	TObjectPtr<UARPGItemDefinition> Definition;

	/** Current stack count. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1"), SaveGame, Replicated)
	int32 StackCount = 1;

	/** Item level — affects affix scaling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1"), SaveGame, Replicated)
	int32 ItemLevel = 1;

	/**
	 * Rarity rolled at generation time (may differ from Definition->Rarity).
	 * Use GetEffectiveRarity() to get the correct rarity for visuals/logic.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", SaveGame, Replicated)
	EARPGItemRarity RolledRarity = EARPGItemRarity::Common;

	/** Whether RolledRarity was explicitly set (vs defaulting to Definition->Rarity). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item", SaveGame, Replicated)
	bool bHasRolledRarity = false;

	/** Randomly rolled affixes on this instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", SaveGame, Replicated)
	TArray<FItemAffix> Affixes;

	/** Unique identifier for this item instance. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item", SaveGame, Replicated)
	FGuid UniqueId;

	/** Handle to the active base equip effect on the character's ASC. Invalid when not equipped. */
	FActiveGameplayEffectHandle EquipEffectHandle;

	// --- Serialization ---

	/** Serialize for SaveGame persistence. Saves all UPROPERTY(SaveGame) fields. */
	virtual void Serialize(FArchive& Ar) override;

	/** Create a lightweight save data snapshot of this instance. */
	UFUNCTION(BlueprintCallable, Category = "Item|Save")
	FARPGItemSaveData ToSaveData() const;

	/** Restore this instance's state from save data. Requires the definition to be loaded externally. */
	UFUNCTION(BlueprintCallable, Category = "Item|Save")
	void FromSaveData(const FARPGItemSaveData& SaveData, UARPGItemDefinition* InDefinition);

	// --- Active Effect Management ---

	/** Returns true if any equip/affix effects are currently active on an ASC. */
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool HasActiveEffects() const;

	/** Invalidate all active effect handles (call after effects have been removed from ASC). */
	void ClearActiveHandles();

	// --- Helpers ---

	/** Returns the display name with affix prefixes/suffixes (e.g., "Blazing Iron Sword of Strength"). */
	UFUNCTION(BlueprintCallable, Category = "Item")
	FText GetDisplayName() const;

	/** Returns the sum of all affix magnitudes. */
	UFUNCTION(BlueprintCallable, Category = "Item")
	float GetTotalStats() const;

	/** Returns true if this item can accept more units in its stack. */
	UFUNCTION(BlueprintCallable, Category = "Item")
	bool CanStack() const;

	/** Build the full item name with affix prefix/suffix decorations. */
	FText BuildAffixName() const;

	/** Returns the effective rarity — RolledRarity if set, otherwise Definition->Rarity. */
	UFUNCTION(BlueprintPure, Category = "Item")
	EARPGItemRarity GetEffectiveRarity() const;
};

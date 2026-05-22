#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Inventory/ARPGItemDefinition.h"
#include "ARPGAffixTypes.generated.h"

class UGameplayEffect;

// =========================================================================
// Data Table row — one possible affix that can roll on an item
// =========================================================================

USTRUCT(BlueprintType)
struct FAffixTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Identifying tag for this affix (e.g., Affix.Strength, Affix.CritChance). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	FGameplayTag AffixTag;

	/** Display suffix/prefix for item naming (e.g., "of Strength", "Blazing"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	FText DisplayName;

	/** Whether this is a prefix ("Blazing Sword") or suffix ("Sword of Strength"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	bool bIsPrefix = false;

	/** Minimum rolled magnitude value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	float MinValue = 1.f;

	/** Maximum rolled magnitude value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	float MaxValue = 10.f;

	/** Weight for weighted random selection. Higher = more likely to roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix", meta = (ClampMin = "0.01"))
	float Weight = 1.f;

	/** Minimum rarity required for this affix to appear. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	EARPGItemRarity MinRarity = EARPGItemRarity::Common;

	/** The gameplay effect class this affix applies (infinite duration, magnitude via SetByCaller). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affix")
	TSubclassOf<UGameplayEffect> Effect;
};

// =========================================================================
// Rarity-tier affix count configuration
// =========================================================================

struct FRarityAffixRange
{
	int32 MinAffixes;
	int32 MaxAffixes;
};

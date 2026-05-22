#include "Inventory/ARPGLootTable.h"
#include "Inventory/ARPGItemInstance.h"
#include "Inventory/ARPGItemDefinition.h"
#include "Inventory/ARPGAffixRoller.h"

// Base rarity weights — exponentially decreasing for higher tiers
static const float GRarityBaseWeights[] = { 100.f, 50.f, 20.f, 5.f, 1.f };
static const int32 GNumRarityTiers = UE_ARRAY_COUNT(GRarityBaseWeights);

EARPGItemRarity UARPGLootTable::RollWeightedRarity(EARPGItemRarity MinRarity, EARPGItemRarity MaxRarity, float RarityBonusMultiplier)
{
	const int32 MinIdx = FMath::Clamp(static_cast<int32>(MinRarity), 0, GNumRarityTiers - 1);
	const int32 MaxIdx = FMath::Clamp(static_cast<int32>(FMath::Max(MinRarity, MaxRarity)), MinIdx, GNumRarityTiers - 1);

	if (MinIdx == MaxIdx)
	{
		return static_cast<EARPGItemRarity>(MinIdx);
	}

	// Build weighted distribution. Bonus multiplier raises higher-tier weights.
	// Tier 0 (Common) weight is unchanged; each tier above gets bonus^tier_offset applied.
	float TotalWeight = 0.f;
	float Weights[5];
	for (int32 i = MinIdx; i <= MaxIdx; ++i)
	{
		float W = GRarityBaseWeights[i];
		if (RarityBonusMultiplier > 1.f && i > MinIdx)
		{
			// Boost = bonus^(distance from min), applied multiplicatively to the base weight
			const float TierOffset = static_cast<float>(i - MinIdx);
			W *= FMath::Pow(RarityBonusMultiplier, TierOffset);
		}
		Weights[i] = W;
		TotalWeight += W;
	}

	const float Roll = FMath::FRandRange(0.f, TotalWeight);
	float Accumulated = 0.f;
	for (int32 i = MinIdx; i <= MaxIdx; ++i)
	{
		Accumulated += Weights[i];
		if (Roll < Accumulated)
		{
			return static_cast<EARPGItemRarity>(i);
		}
	}

	return static_cast<EARPGItemRarity>(MaxIdx);
}

TArray<UARPGItemInstance*> UARPGLootTable::RollLoot(UObject* Outer, float RarityBonusMultiplier) const
{
	TArray<UARPGItemInstance*> Results;

	if (Entries.Num() == 0)
	{
		return Results;
	}

	// Calculate total weight including the "nothing" weight
	float TotalWeight = NothingWeight;
	for (const FLootEntry& Entry : Entries)
	{
		if (Entry.Item && Entry.DropWeight > 0.f)
		{
			TotalWeight += Entry.DropWeight;
		}
	}

	if (TotalWeight <= 0.f)
	{
		return Results;
	}

	// Roll a random value in [0, TotalWeight)
	const float Roll = FMath::FRandRange(0.f, TotalWeight);

	// Walk through entries accumulating weight to find the selected one
	float Accumulated = NothingWeight;
	if (Roll < Accumulated)
	{
		return Results;
	}

	for (const FLootEntry& Entry : Entries)
	{
		if (!Entry.Item || Entry.DropWeight <= 0.f)
		{
			continue;
		}

		Accumulated += Entry.DropWeight;
		if (Roll < Accumulated)
		{
			const int32 Quantity = FMath::RandRange(Entry.MinQuantity, FMath::Max(Entry.MinQuantity, Entry.MaxQuantity));

			// Weighted rarity roll — higher rarities are exponentially rarer
			const EARPGItemRarity RolledRarity = RollWeightedRarity(
				Entry.MinRarity, Entry.MaxRarity, FMath::Max(1.f, RarityBonusMultiplier));

			UObject* OuterObj = Outer ? Outer : GetTransientPackage();
			for (int32 i = 0; i < Quantity; ++i)
			{
				UARPGItemInstance* Instance = NewObject<UARPGItemInstance>(OuterObj);
				Instance->Definition = Entry.Item;
				Instance->StackCount = 1;
				Instance->ItemLevel = 1;
				Instance->UniqueId = FGuid::NewGuid();
				Instance->RolledRarity = RolledRarity;
				Instance->bHasRolledRarity = true;

				if (Entry.Item->AffixPool)
				{
					UARPGAffixRoller::RollAffixes(Entry.Item->AffixPool, RolledRarity, Instance->ItemLevel, Instance->Affixes);
				}

				Results.Add(Instance);
			}
			break;
		}
	}

	return Results;
}

FPrimaryAssetId UARPGLootTable::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ARPGLootTable"), GetFName());
}

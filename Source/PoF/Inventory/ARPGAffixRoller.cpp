#include "Inventory/ARPGAffixRoller.h"
#include "Inventory/ARPGAffixTypes.h"
#include "Inventory/ARPGItemInstance.h"
#include "Inventory/ARPGItemDefinition.h"
#include "Engine/DataTable.h"

void UARPGAffixRoller::GetAffixCountRange(EARPGItemRarity Rarity, int32& OutMin, int32& OutMax)
{
	switch (Rarity)
	{
	case EARPGItemRarity::Common:
		OutMin = 0; OutMax = 0; break;
	case EARPGItemRarity::Uncommon:
		OutMin = 1; OutMax = 2; break;
	case EARPGItemRarity::Rare:
		OutMin = 3; OutMax = 4; break;
	case EARPGItemRarity::Epic:
		OutMin = 4; OutMax = 5; break;
	case EARPGItemRarity::Legendary:
		OutMin = 5; OutMax = 6; break;
	default:
		OutMin = 0; OutMax = 0; break;
	}
}

float UARPGAffixRoller::GetItemLevelScaling(int32 ItemLevel)
{
	return 1.f + 0.1f * static_cast<float>(FMath::Max(1, ItemLevel));
}

void UARPGAffixRoller::RollAffixes(const UDataTable* AffixPool, EARPGItemRarity Rarity, int32 ItemLevel, TArray<FItemAffix>& OutAffixes)
{
	OutAffixes.Reset();

	if (!AffixPool)
	{
		return;
	}

	int32 MinCount, MaxCount;
	GetAffixCountRange(Rarity, MinCount, MaxCount);

	if (MaxCount <= 0)
	{
		return;
	}

	// Gather all valid rows, filtering by tier-gate (MinRarity)
	TArray<FAffixTableRow*> AllRows;
	AffixPool->GetAllRows<FAffixTableRow>(TEXT("RollAffixes"), AllRows);

	TArray<FAffixTableRow*> EligibleRows;
	EligibleRows.Reserve(AllRows.Num());

	for (FAffixTableRow* Row : AllRows)
	{
		if (Row && Row->MinRarity <= Rarity)
		{
			EligibleRows.Add(Row);
		}
	}

	if (EligibleRows.IsEmpty())
	{
		return;
	}

	const int32 DesiredCount = FMath::RandRange(MinCount, MaxCount);
	const int32 ActualCount = FMath::Min(DesiredCount, EligibleRows.Num());
	const float LevelScale = GetItemLevelScaling(ItemLevel);

	// Build a weighted selection pool (indices into EligibleRows + cumulative weights)
	TArray<float> CumulativeWeights;
	CumulativeWeights.SetNum(EligibleRows.Num());

	float TotalWeight = 0.f;
	for (int32 i = 0; i < EligibleRows.Num(); ++i)
	{
		TotalWeight += FMath::Max(0.01f, EligibleRows[i]->Weight);
		CumulativeWeights[i] = TotalWeight;
	}

	// Weighted selection without replacement
	TSet<int32> ChosenIndices;
	OutAffixes.Reserve(ActualCount);

	for (int32 Picked = 0; Picked < ActualCount && ChosenIndices.Num() < EligibleRows.Num(); ++Picked)
	{
		// Rebuild effective weight sum excluding already-chosen rows
		float EffectiveTotal = 0.f;
		TArray<TPair<int32, float>> Pool;
		Pool.Reserve(EligibleRows.Num() - ChosenIndices.Num());

		for (int32 i = 0; i < EligibleRows.Num(); ++i)
		{
			if (!ChosenIndices.Contains(i))
			{
				const float W = FMath::Max(0.01f, EligibleRows[i]->Weight);
				EffectiveTotal += W;
				Pool.Emplace(i, EffectiveTotal);
			}
		}

		if (Pool.IsEmpty() || EffectiveTotal <= 0.f)
		{
			break;
		}

		const float Roll = FMath::FRandRange(0.f, EffectiveTotal);
		int32 SelectedIdx = Pool.Last().Key;

		for (const auto& Entry : Pool)
		{
			if (Roll <= Entry.Value)
			{
				SelectedIdx = Entry.Key;
				break;
			}
		}

		ChosenIndices.Add(SelectedIdx);

		const FAffixTableRow* Row = EligibleRows[SelectedIdx];

		FItemAffix Affix;
		Affix.AffixTag = Row->AffixTag;
		Affix.Effect = Row->Effect;
		Affix.Magnitude = FMath::FRandRange(Row->MinValue, Row->MaxValue) * LevelScale;
		Affix.DisplayName = Row->DisplayName;
		Affix.bIsPrefix = Row->bIsPrefix;

		OutAffixes.Add(MoveTemp(Affix));
	}
}

bool UARPGAffixRoller::RerollAffixes(UARPGItemInstance* Item)
{
	if (!Item || !Item->Definition || !Item->Definition->AffixPool)
	{
		return false;
	}

	Item->Affixes.Reset();
	RollAffixes(Item->Definition->AffixPool, Item->Definition->Rarity, Item->ItemLevel, Item->Affixes);
	return Item->Affixes.Num() > 0;
}

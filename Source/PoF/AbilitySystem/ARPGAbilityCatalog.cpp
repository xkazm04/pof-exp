// Phase 2 F3b — implementation.

#include "ARPGAbilityCatalog.h"
#include "Engine/DataTable.h"
#include "UObject/ConstructorHelpers.h"

const TCHAR* UARPGAbilityCatalog::GetCatalogAssetPath()
{
	return TEXT("/Game/Abilities/DT_AbilityCatalog.DT_AbilityCatalog");
}

UDataTable* UARPGAbilityCatalog::GetCatalogDataTable()
{
	// LoadObject is cached internally by the asset registry; for a small catalog
	// the per-call cost is negligible. Promote to a static cached weak pointer
	// only if profiling shows it.
	return LoadObject<UDataTable>(nullptr, GetCatalogAssetPath());
}

bool UARPGAbilityCatalog::GetAbilityRowByTag(FGameplayTag Tag, FARPGAbilityCatalogRow& OutRow)
{
	if (!Tag.IsValid()) return false;

	UDataTable* DT = GetCatalogDataTable();
	if (!DT) return false;

	for (const auto& Pair : DT->GetRowMap())
	{
		const FARPGAbilityCatalogRow* Row = reinterpret_cast<const FARPGAbilityCatalogRow*>(Pair.Value);
		if (Row && Row->GameplayTag == Tag)
		{
			OutRow = *Row;
			return true;
		}
	}
	return false;
}

bool UARPGAbilityCatalog::GetAbilityRowByName(FName RowName, FARPGAbilityCatalogRow& OutRow)
{
	UDataTable* DT = GetCatalogDataTable();
	if (!DT) return false;

	if (const FARPGAbilityCatalogRow* Row = DT->FindRow<FARPGAbilityCatalogRow>(RowName, TEXT("UARPGAbilityCatalog::GetAbilityRowByName")))
	{
		OutRow = *Row;
		return true;
	}
	return false;
}

TArray<FARPGAbilityCatalogRow> UARPGAbilityCatalog::GetAllAbilityRows()
{
	TArray<FARPGAbilityCatalogRow> Out;
	UDataTable* DT = GetCatalogDataTable();
	if (!DT) return Out;

	for (const auto& Pair : DT->GetRowMap())
	{
		if (const FARPGAbilityCatalogRow* Row = reinterpret_cast<const FARPGAbilityCatalogRow*>(Pair.Value))
		{
			Out.Add(*Row);
		}
	}
	return Out;
}

TArray<FARPGAbilityCatalogRow> UARPGAbilityCatalog::GetAbilityRowsByCategory(EARPGAbilityCategory Category)
{
	TArray<FARPGAbilityCatalogRow> Out;
	UDataTable* DT = GetCatalogDataTable();
	if (!DT) return Out;

	for (const auto& Pair : DT->GetRowMap())
	{
		const FARPGAbilityCatalogRow* Row = reinterpret_cast<const FARPGAbilityCatalogRow*>(Pair.Value);
		if (Row && Row->Category == Category)
		{
			Out.Add(*Row);
		}
	}
	return Out;
}

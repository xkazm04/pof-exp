// Phase 2 F3b — Blueprint-callable lookup helper for the ability catalog DataTable.
//
// See ARPGAbilityCatalogTypes.h for the SYNC SOURCE relationship with the PoF
// app's AbilityMeta schema.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "ARPGAbilityCatalogTypes.h"
#include "ARPGAbilityCatalog.generated.h"

class UDataTable;

/**
 * Static helpers for reading rows out of DT_AbilityCatalog.
 *
 * The catalog asset path is hardcoded for v1 (/Game/Abilities/DT_AbilityCatalog);
 * later move to UARPGDeveloperSettings if multiple DataTables are needed.
 */
UCLASS()
class POF_API UARPGAbilityCatalog : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Soft path to the catalog DataTable. */
	static const TCHAR* GetCatalogAssetPath();

	/** Cached load of the DataTable — returns nullptr if missing. */
	UFUNCTION(BlueprintCallable, Category = "Ability|Catalog")
	static UDataTable* GetCatalogDataTable();

	/**
	 * Look up a row by its matching FGameplayTag (linear scan — the catalog is small).
	 * Returns true and fills OutRow on hit; false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Catalog", meta = (DisplayName = "Get Ability Row By Tag"))
	static bool GetAbilityRowByTag(FGameplayTag Tag, FARPGAbilityCatalogRow& OutRow);

	/** Look up a row by its row name (the AbilityMeta.id mirror). */
	UFUNCTION(BlueprintCallable, Category = "Ability|Catalog")
	static bool GetAbilityRowByName(FName RowName, FARPGAbilityCatalogRow& OutRow);

	/** All rows currently in the catalog, in iteration order. */
	UFUNCTION(BlueprintCallable, Category = "Ability|Catalog")
	static TArray<FARPGAbilityCatalogRow> GetAllAbilityRows();

	/** Filter helper — all rows whose Category matches. */
	UFUNCTION(BlueprintCallable, Category = "Ability|Catalog")
	static TArray<FARPGAbilityCatalogRow> GetAbilityRowsByCategory(EARPGAbilityCategory Category);
};

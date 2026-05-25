#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ARPGCurrencyTypes.generated.h"

/**
 * Data-driven definition of one spendable currency, designed to live as a row
 * in a DT_Currencies UDataTable (row name == CurrencyId). Mirrors the project's
 * FTableRowBase convention (cf. FAffixTableRow / FARPGAttributeInitRow).
 *
 * Catalog pipeline (docs/catalog/economy-meta/currency): this is the "Currency"
 * catalog's schema; the seeded target asset is the "Gold" row. Fields map 1:1 to
 * the brief's pipeline steps:
 *   - Cap / DecayPerDay        -> step 3  (Cap & Decay Rules)
 *   - BaseUnitValue            -> step 4  (Conversion Rules)
 *   - DisplayName / Plural     -> step 10 (Localization keys)
 *   - Category                 -> step 2  (Source/Sink Mapping bucket)
 */
USTRUCT(BlueprintType)
struct POF_API FARPGCurrencyDef : public FTableRowBase
{
	GENERATED_BODY()

	/** Stable identity; also the DataTable row name. e.g. "Gold". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency")
	FName CurrencyId;

	/** Singular display name (the localization handle, step 10). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency")
	FText DisplayName;

	/** Plural display name (countable currencies differ; Gold is uncountable). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency")
	FText DisplayNamePlural;

	/** Economy bucket — e.g. "Standard" (soft) vs "Premium". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency")
	FName Category = TEXT("Standard");

	/** Maximum holdable balance; 0 = uncapped. (Step 3 — Cap rule.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency", meta = (ClampMin = "0"))
	int32 Cap = 0;

	/** Fraction of the balance lost per in-game day; 0 = no decay. (Step 3 — Decay rule.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DecayPerDay = 0.f;

	/** Worth of one unit in the base currency (Gold = 1.0); drives conversion. (Step 4.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency", meta = (ClampMin = "0.0"))
	float BaseUnitValue = 1.f;
};

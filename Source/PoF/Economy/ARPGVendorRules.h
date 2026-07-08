#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGVendorRules.generated.h"

/**
 * Vendor pricing rules (catalog pipeline: vendors). Canon vendor-laws: 30% markup
 * over theoretical item cost (±20% band → 24–36%), buyback at 50% of sell price,
 * settlement in gold only, reputation discount applied on buy (reuses the faction
 * discount ladder). Mirrors src/lib/catalog/pipelines/vendors.ts.
 */
UCLASS()
class POF_API UARPGVendorRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr int32 MarkupPct = 30;
	static constexpr int32 BuybackPct = 50;

	/** Price a player pays to buy: base cost + 30% markup, then the tier discount. */
	UFUNCTION(BlueprintPure, Category = "Vendor")
	static int32 BuyPrice(int32 BaseCost, int32 DiscountPercent);

	/** Price a player receives selling to the vendor (base cost, no markup). */
	UFUNCTION(BlueprintPure, Category = "Vendor")
	static int32 SellPrice(int32 BaseCost) { return FMath::Max(BaseCost, 0); }

	/** Buyback price = 50% of the price the item was sold at. */
	UFUNCTION(BlueprintPure, Category = "Vendor")
	static int32 BuybackPrice(int32 SoldAtPrice);
};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Economy/ARPGFactionRules.h"
#include "ARPGVendorComponent.generated.h"

/**
 * One stock line of a vendor's inventory, designed to live as a row in a
 * DT_VendorInventory UDataTable (row name == the vendor id). Catalog pipeline:
 * vendors → UE Packaging. Mirrors src/lib/catalog/pipelines/vendors.ts.
 */
USTRUCT(BlueprintType)
struct POF_API FARPGVendorInventoryRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Item id sold on this line (resolves against the items catalog). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendor")
	FName ItemId;

	/** Theoretical item cost the 30% markup is applied over. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendor", meta = (ClampMin = "0"))
	int32 BaseCost = 0;

	/** Units in stock; -1 = unlimited. Restocks on the vendor's restock interval. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendor")
	int32 Stock = -1;
};

/**
 * Vendor runtime component (catalog pipeline: vendors → UE Packaging). Prices buy/sell/
 * buyback/repair through the shared UARPGVendorRules (30% markup, 50% buyback) and applies
 * the reputation discount via UARPGFactionRules — the same rules the L3 gate
 * (VSVendorTransactionTest) asserts, so runtime and gate agree.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class POF_API UARPGVendorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Data table of this vendor's stock (rows of FARPGVendorInventoryRow). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendor")
	TObjectPtr<UDataTable> InventoryTable;

	/** Price the buyer pays for a stock line at the given faction tier (markup + discount). */
	UFUNCTION(BlueprintPure, Category = "Vendor")
	int32 GetBuyPrice(const FARPGVendorInventoryRow& Line, EARPGFactionTier Tier) const;

	/** Price the vendor pays the player for an item of the given base cost. */
	UFUNCTION(BlueprintPure, Category = "Vendor")
	int32 GetSellPrice(int32 BaseCost) const;

	/** Buyback price for an item the player previously sold at SoldAtPrice. */
	UFUNCTION(BlueprintPure, Category = "Vendor")
	int32 GetBuybackPrice(int32 SoldAtPrice) const;
};

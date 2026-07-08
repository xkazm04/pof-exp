#include "Economy/ARPGVendorComponent.h"
#include "Economy/ARPGVendorRules.h"

int32 UARPGVendorComponent::GetBuyPrice(const FARPGVendorInventoryRow& Line, EARPGFactionTier Tier) const
{
	const int32 Discount = UARPGFactionRules::GetDiscountPercent(Tier);
	return UARPGVendorRules::BuyPrice(Line.BaseCost, Discount);
}

int32 UARPGVendorComponent::GetSellPrice(int32 BaseCost) const
{
	return UARPGVendorRules::SellPrice(BaseCost);
}

int32 UARPGVendorComponent::GetBuybackPrice(int32 SoldAtPrice) const
{
	return UARPGVendorRules::BuybackPrice(SoldAtPrice);
}

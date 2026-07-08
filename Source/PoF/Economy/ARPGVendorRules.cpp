#include "Economy/ARPGVendorRules.h"

int32 UARPGVendorRules::BuyPrice(int32 BaseCost, int32 DiscountPercent)
{
	const double Marked = static_cast<double>(BaseCost) * (1.0 + static_cast<double>(MarkupPct) / 100.0);
	const double Discounted = Marked * (1.0 - static_cast<double>(DiscountPercent) / 100.0);
	return FMath::RoundToInt(Discounted);
}

int32 UARPGVendorRules::BuybackPrice(int32 SoldAtPrice)
{
	return FMath::RoundToInt(static_cast<double>(SoldAtPrice) * static_cast<double>(BuybackPct) / 100.0);
}

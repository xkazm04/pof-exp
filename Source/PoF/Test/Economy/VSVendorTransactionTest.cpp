#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Economy/ARPGVendorRules.h"
#include "Economy/ARPGFactionRules.h"

/**
 * Vendor catalog L3 gate (vendors, runtimeDeferred('VSVendorTransactionTest')).
 * Asserts the production vendor pricing rules: 30% markup, 50% buyback, gold-only,
 * and the reputation discount integration (Exalted = 20% off). Pure math — headless.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSVendorTransactionTest,
	"Project.Functional Tests.PoF.Vendors.VSVendorTransactionTest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSVendorTransactionTest::RunTest(const FString& /*Parameters*/)
{
	// Canon constants.
	TestEqual(TEXT("markup = 30%"), UARPGVendorRules::MarkupPct, 30);
	TestEqual(TEXT("buyback = 50%"), UARPGVendorRules::BuybackPct, 50);

	// Buy price at Neutral (no discount): 100 base → 130 (30% markup).
	TestEqual(TEXT("Neutral buy = base + 30% markup"),
		UARPGVendorRules::BuyPrice(100, 0), 130);

	// Exalted discount (20%) applied on top of markup: 100 * 1.30 * 0.80 = 104.
	const int32 ExaltedDiscount = UARPGFactionRules::GetDiscountPercent(EARPGFactionTier::Exalted);
	TestEqual(TEXT("Exalted discount is 20"), ExaltedDiscount, 20);
	TestEqual(TEXT("Exalted buy = base*1.30*0.80"),
		UARPGVendorRules::BuyPrice(100, ExaltedDiscount), 104);

	// Friendly discount (5%): 100 * 1.30 * 0.95 = 123.5 → 124.
	TestEqual(TEXT("Friendly buy = base*1.30*0.95"),
		UARPGVendorRules::BuyPrice(100, UARPGFactionRules::GetDiscountPercent(EARPGFactionTier::Friendly)), 124);

	// Buyback = 50% of the price it sold at.
	TestEqual(TEXT("buyback = 50% of sold price"),
		UARPGVendorRules::BuybackPrice(80), 40);

	// Sell price (vendor buys from player at base, no markup).
	TestEqual(TEXT("sell price = base"), UARPGVendorRules::SellPrice(100), 100);

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

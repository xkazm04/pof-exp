#include "Test/Economy/VSCurrencyWalletTest.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Economy/ARPGWalletComponent.h"
#include "Economy/ARPGCurrencyTypes.h"

/**
 * Economy Test Gate for the "Currency" catalog — target asset Gold.
 * docs/catalog/economy-meta/currency/plan.md, step 13.
 *
 * Pure logic gate (no PIE/world/map needed — safe under shared-tree
 * concurrency). Drives a UARPGWalletComponent through the economy rules the
 * brief requires: faucet credit, CanAfford boundary, cap clamp, daily decay,
 * anti-exploit overspend/negative rejection, atomic conversion, and the
 * telemetry delegate firing. Mirrors the Fireball effect-config gate.
 *
 * Runs headless via:
 *   UnrealEditor-Cmd PoF.uproject -ExecCmds="Automation RunTests Project.Functional Tests.PoF.Currency;Quit" -unattended -nopause -nullrhi -log
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVSCurrencyWalletTest,
	"Project.Functional Tests.PoF.Currency.WalletRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVSCurrencyWalletTest::RunTest(const FString& /*Parameters*/)
{
	UARPGWalletComponent* Wallet = NewObject<UARPGWalletComponent>(GetTransientPackage());
	if (!TestNotNull(TEXT("Wallet component created"), Wallet))
	{
		return false;
	}

	UVSWalletChangeCounter* Counter = NewObject<UVSWalletChangeCounter>(GetTransientPackage());
	if (!TestNotNull(TEXT("Telemetry counter created"), Counter))
	{
		return false;
	}
	Wallet->OnCurrencyChanged.AddDynamic(Counter, &UVSWalletChangeCounter::OnChanged);

	static const FName Gold(TEXT("Gold"));

	// Gold: soft currency, cap 120, 10%/day decay, base value 1.0.
	FARPGCurrencyDef GoldDef;
	GoldDef.CurrencyId = Gold;
	GoldDef.DisplayName = FText::FromString(TEXT("Gold"));
	GoldDef.DisplayNamePlural = FText::FromString(TEXT("Gold"));
	GoldDef.Category = TEXT("Standard");
	GoldDef.Cap = 120;
	GoldDef.DecayPerDay = 0.10f;
	GoldDef.BaseUnitValue = 1.f;
	Wallet->RegisterCurrency(GoldDef);

	// 1. Fresh wallet starts empty and registered.
	TestEqual(TEXT("Gold balance starts at 0"), Wallet->GetBalance(Gold), 0);
	TestTrue(TEXT("Gold is registered"), Wallet->IsCurrencyRegistered(Gold));

	// 2. Faucet credit.
	TestEqual(TEXT("AddCurrency(100) credits 100"), Wallet->AddCurrency(Gold, 100), 100);
	TestEqual(TEXT("Balance is 100 after credit"), Wallet->GetBalance(Gold), 100);

	// 3. CanAfford boundary.
	TestTrue(TEXT("CanAfford(100) at balance 100"), Wallet->CanAfford(Gold, 100));
	TestFalse(TEXT("!CanAfford(101) at balance 100"), Wallet->CanAfford(Gold, 101));

	// 4. Sink debit.
	TestTrue(TEXT("SpendCurrency(30) succeeds"), Wallet->SpendCurrency(Gold, 30));
	TestEqual(TEXT("Balance is 70 after spend"), Wallet->GetBalance(Gold), 70);

	// 5. Anti-exploit: overspend rejected atomically.
	TestFalse(TEXT("Overspend rejected"), Wallet->SpendCurrency(Gold, 1000));
	TestEqual(TEXT("Balance unchanged after rejected overspend"), Wallet->GetBalance(Gold), 70);

	// 6. Anti-exploit: non-positive amounts rejected.
	TestFalse(TEXT("Negative spend rejected"), Wallet->SpendCurrency(Gold, -5));
	TestEqual(TEXT("Negative add rejected (0 credited)"), Wallet->AddCurrency(Gold, -5), 0);
	TestEqual(TEXT("Balance unchanged after rejected non-positive ops"), Wallet->GetBalance(Gold), 70);

	// 7. Cap clamp: 70 + 100 -> clamp to 120, crediting only 50.
	TestEqual(TEXT("AddCurrency past cap credits only up to the cap (50)"), Wallet->AddCurrency(Gold, 100), 50);
	TestEqual(TEXT("Balance clamps to cap 120"), Wallet->GetBalance(Gold), 120);

	// 8. Daily decay: 10% of 120 -> keep 108.
	Wallet->ApplyDailyDecay();
	TestEqual(TEXT("Daily decay (10%) brings 120 -> 108"), Wallet->GetBalance(Gold), 108);

	// 9. Telemetry hook fired on each real change (credit, spend, capped credit, decay = 4).
	TestTrue(
		FString::Printf(TEXT("OnCurrencyChanged fired on each change (got %d, expected >= 4)"), Counter->Count),
		Counter->Count >= 4);

	// 10. Conversion in a clean uncapped wallet: 1 Shard = 100 Gold.
	UARPGWalletComponent* Wallet2 = NewObject<UARPGWalletComponent>(GetTransientPackage());
	static const FName Shard(TEXT("Shard"));
	FARPGCurrencyDef GoldUncapped = GoldDef;
	GoldUncapped.Cap = 0;
	GoldUncapped.DecayPerDay = 0.f;
	FARPGCurrencyDef ShardDef;
	ShardDef.CurrencyId = Shard;
	ShardDef.BaseUnitValue = 100.f;
	Wallet2->RegisterCurrency(GoldUncapped);
	Wallet2->RegisterCurrency(ShardDef);
	Wallet2->AddCurrency(Shard, 5);

	TestEqual(TEXT("Converting 2 Shards (x100) credits 200 Gold"), Wallet2->Convert(Shard, Gold, 2), 200);
	TestEqual(TEXT("Shard balance drops to 3 after converting 2"), Wallet2->GetBalance(Shard), 3);
	TestEqual(TEXT("Gold balance is 200 after conversion"), Wallet2->GetBalance(Gold), 200);

	// 11. Conversion with insufficient source is atomic (returns -1, nothing moves).
	TestEqual(TEXT("Converting more Shards than held fails (-1)"), Wallet2->Convert(Shard, Gold, 99), -1);
	TestEqual(TEXT("Shard unchanged after failed conversion"), Wallet2->GetBalance(Shard), 3);
	TestEqual(TEXT("Gold unchanged after failed conversion"), Wallet2->GetBalance(Gold), 200);

	return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS

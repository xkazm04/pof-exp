#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Economy/ARPGCurrencyTypes.h"
#include "ARPGWalletComponent.generated.h"

/**
 * Fires on every balance change — the single telemetry / wallet-UI hook
 * (pipeline steps 9 & 12). Params: currency id, the new balance, and the
 * signed delta that produced it.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCurrencyChanged, FName, CurrencyId, int32, NewBalance, int32, Delta);

/**
 * Holds an actor's currency balances and enforces the economy rules each
 * currency carries: cap clamping, daily decay, conversion, and the anti-exploit
 * guards on add/spend. Modelled on UARPGInventoryComponent.
 *
 * Sources (faucets: kills, quests, vendor-sales, chests) call AddCurrency;
 * sinks (potions, repairs, vendor-buys, fees) call SpendCurrency — the same
 * faucet/sink taxonomy the app-side economy simulator balances
 * (src/lib/economy/definitions.ts). The existing UARPGLootDropComponent gold
 * drop is the first faucet that should route through AddCurrency once a wallet
 * lives on the player.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class POF_API UARPGWalletComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGWalletComponent();

	/** Register (or replace) a currency this wallet can hold. Safe to call repeatedly. */
	UFUNCTION(BlueprintCallable, Category = "Wallet")
	void RegisterCurrency(const FARPGCurrencyDef& Def);

	/** True once the currency has been registered. */
	UFUNCTION(BlueprintCallable, Category = "Wallet")
	bool IsCurrencyRegistered(FName CurrencyId) const;

	/** Current balance (0 if the currency is unknown or empty). */
	UFUNCTION(BlueprintCallable, Category = "Wallet")
	int32 GetBalance(FName CurrencyId) const;

	/**
	 * Credit a faucet amount, clamped to the currency Cap.
	 * Rejects unknown currencies and non-positive amounts (anti-exploit, step 11).
	 * @return units actually credited (Amount minus any lost to the cap; 0 on reject).
	 */
	UFUNCTION(BlueprintCallable, Category = "Wallet")
	int32 AddCurrency(FName CurrencyId, int32 Amount);

	/**
	 * Debit a sink amount. Atomic: changes nothing and returns false if the
	 * balance is insufficient or the amount is non-positive (anti-exploit, step 11).
	 */
	UFUNCTION(BlueprintCallable, Category = "Wallet")
	bool SpendCurrency(FName CurrencyId, int32 Amount);

	/** Whether the wallet currently holds at least Amount of the currency. */
	UFUNCTION(BlueprintCallable, Category = "Wallet")
	bool CanAfford(FName CurrencyId, int32 Amount) const;

	/**
	 * Convert SourceAmount of From into To using their BaseUnitValue ratio.
	 * Debits From and credits To (subject to To's cap). Atomic — on any failure
	 * (unknown currency, same currency, non-positive amount, insufficient balance,
	 * sub-unit result) nothing changes and -1 is returned. (Step 4 — Conversion.)
	 * @return units of To credited, or -1 on failure.
	 */
	UFUNCTION(BlueprintCallable, Category = "Wallet")
	int32 Convert(FName FromCurrencyId, FName ToCurrencyId, int32 SourceAmount);

	/** Apply each currency's DecayPerDay once (call per in-game day). (Step 3 — Decay.) */
	UFUNCTION(BlueprintCallable, Category = "Wallet")
	void ApplyDailyDecay();

	/** Telemetry / wallet-UI hook (steps 9 & 12) — fires on every balance change. */
	UPROPERTY(BlueprintAssignable, Category = "Wallet")
	FOnCurrencyChanged OnCurrencyChanged;

private:
	/** Set a balance, fire the change delegate when Delta != 0, return the new balance. */
	int32 SetBalanceInternal(FName CurrencyId, int32 NewBalance, int32 Delta);

	/** Registered currency definitions, keyed by CurrencyId. */
	TMap<FName, FARPGCurrencyDef> Defs;

	/** Live balances, keyed by CurrencyId. */
	TMap<FName, int32> Balances;
};

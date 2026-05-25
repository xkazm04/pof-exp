#include "Economy/ARPGWalletComponent.h"

UARPGWalletComponent::UARPGWalletComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UARPGWalletComponent::RegisterCurrency(const FARPGCurrencyDef& Def)
{
	if (Def.CurrencyId.IsNone())
	{
		return;
	}
	Defs.Add(Def.CurrencyId, Def);
	if (!Balances.Contains(Def.CurrencyId))
	{
		Balances.Add(Def.CurrencyId, 0);
	}
}

bool UARPGWalletComponent::IsCurrencyRegistered(FName CurrencyId) const
{
	return Defs.Contains(CurrencyId);
}

int32 UARPGWalletComponent::GetBalance(FName CurrencyId) const
{
	const int32* Found = Balances.Find(CurrencyId);
	return Found ? *Found : 0;
}

int32 UARPGWalletComponent::SetBalanceInternal(FName CurrencyId, int32 NewBalance, int32 Delta)
{
	Balances.Add(CurrencyId, NewBalance);
	if (Delta != 0)
	{
		OnCurrencyChanged.Broadcast(CurrencyId, NewBalance, Delta);
	}
	return NewBalance;
}

int32 UARPGWalletComponent::AddCurrency(FName CurrencyId, int32 Amount)
{
	const FARPGCurrencyDef* Def = Defs.Find(CurrencyId);
	if (!Def || Amount <= 0)
	{
		// Unknown currency or non-positive amount — reject (anti-exploit).
		return 0;
	}

	const int32 Current = GetBalance(CurrencyId);
	int32 Target = Current + Amount;

	// Guard against int32 overflow before cap clamping.
	if (Target < Current)
	{
		Target = MAX_int32;
	}

	if (Def->Cap > 0 && Target > Def->Cap)
	{
		Target = Def->Cap;
	}

	const int32 Credited = Target - Current;
	if (Credited <= 0)
	{
		// Already at cap — nothing to credit, no event.
		return 0;
	}

	SetBalanceInternal(CurrencyId, Target, Credited);
	return Credited;
}

bool UARPGWalletComponent::SpendCurrency(FName CurrencyId, int32 Amount)
{
	if (Amount <= 0 || !Defs.Contains(CurrencyId))
	{
		// Non-positive or unknown — reject (anti-exploit).
		return false;
	}

	const int32 Current = GetBalance(CurrencyId);
	if (Current < Amount)
	{
		// Insufficient — atomic, nothing changes.
		return false;
	}

	SetBalanceInternal(CurrencyId, Current - Amount, -Amount);
	return true;
}

bool UARPGWalletComponent::CanAfford(FName CurrencyId, int32 Amount) const
{
	return Amount > 0 && GetBalance(CurrencyId) >= Amount;
}

int32 UARPGWalletComponent::Convert(FName FromCurrencyId, FName ToCurrencyId, int32 SourceAmount)
{
	const FARPGCurrencyDef* From = Defs.Find(FromCurrencyId);
	const FARPGCurrencyDef* To = Defs.Find(ToCurrencyId);
	if (!From || !To || FromCurrencyId == ToCurrencyId || SourceAmount <= 0 || To->BaseUnitValue <= 0.f)
	{
		return -1;
	}
	if (GetBalance(FromCurrencyId) < SourceAmount)
	{
		// Insufficient source — atomic.
		return -1;
	}

	// Worth of the source in base units, then expressed in the target's units.
	const double BaseWorth = static_cast<double>(SourceAmount) * static_cast<double>(From->BaseUnitValue);
	const int32 ToCredit = FMath::FloorToInt(static_cast<float>(BaseWorth / static_cast<double>(To->BaseUnitValue)));
	if (ToCredit <= 0)
	{
		// Sub-unit conversion yields nothing — reject rather than burn the source.
		return -1;
	}

	// Both sides validated, so neither call can partially fail. Credit may be
	// clamped by To's cap (the cap rule still applies on the receiving side).
	SpendCurrency(FromCurrencyId, SourceAmount);
	return AddCurrency(ToCurrencyId, ToCredit);
}

void UARPGWalletComponent::ApplyDailyDecay()
{
	for (const TPair<FName, FARPGCurrencyDef>& Pair : Defs)
	{
		const float Decay = Pair.Value.DecayPerDay;
		if (Decay <= 0.f)
		{
			continue;
		}
		const int32 Current = GetBalance(Pair.Key);
		if (Current <= 0)
		{
			continue;
		}
		const int32 Kept = FMath::FloorToInt(static_cast<float>(Current) * (1.f - Decay));
		if (Kept != Current)
		{
			SetBalanceInternal(Pair.Key, Kept, Kept - Current);
		}
	}
}

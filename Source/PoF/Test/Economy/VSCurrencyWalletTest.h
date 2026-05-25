#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VSCurrencyWalletTest.generated.h"

/**
 * Minimal listener for the economy automation gate. UARPGWalletComponent's
 * OnCurrencyChanged is a dynamic multicast delegate (Blueprint-assignable so
 * wallet UI / telemetry can bind in BP), so counting its broadcasts from the
 * simple-automation test requires a UFUNCTION target on a UObject.
 *
 * See VSCurrencyWalletTest.cpp — the actual gate is an
 * IMPLEMENT_SIMPLE_AUTOMATION_TEST (no map / no PIE), mirroring the Fireball
 * effect-config gate. docs/catalog/economy-meta/currency/plan.md step 13.
 */
UCLASS()
class UVSWalletChangeCounter : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 Count = 0;

	UPROPERTY()
	int32 LastBalance = 0;

	UPROPERTY()
	int32 LastDelta = 0;

	UFUNCTION()
	void OnChanged(FName CurrencyId, int32 NewBalance, int32 Delta)
	{
		++Count;
		LastBalance = NewBalance;
		LastDelta = Delta;
	}
};

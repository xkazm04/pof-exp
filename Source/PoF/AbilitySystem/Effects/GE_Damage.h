#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Damage.generated.h"

/**
 * Instant damage effect using UARPGDamageExecution.
 *
 * Usage: Apply this effect with SetByCaller magnitude for "Data.Damage.Base"
 * to specify the base damage. Optionally set "Data.Damage.Scaling" to control
 * how much AttackPower contributes (defaults to 1.0).
 *
 * The execution calculation handles AttackPower, CritChance, CritDamage,
 * and Armor reduction automatically.
 */
UCLASS()
class POF_API UGE_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Damage();
};

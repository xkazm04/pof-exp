#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "ARPGDamageExecution.generated.h"

/**
 * Custom execution calculation for aRPG damage.
 *
 * Formula:
 *   RawDamage       = BaseDamage + AttackPower * AttackPowerScaling
 *   CritRoll        = FMath::FRand() < CriticalChance
 *   CritMultiplier  = CritRoll ? (1 + CriticalDamage) : 1
 *   ArmorReduction  = Armor / (Armor + 100)        -- diminishing returns
 *   ElemResistance  = target's matching resistance (Fire/Ice/Lightning, 0 for Physical)
 *   FinalDamage     = RawDamage * CritMultiplier * (1 - ArmorReduction) * (1 - ElemResistance)
 *
 * BaseDamage is passed via SetByCaller with tag "Data.Damage.Base".
 * AttackPowerScaling defaults to 1.0 and can be overridden via SetByCaller "Data.Damage.Scaling".
 * Damage type is determined by dynamic asset tags on the GE spec (Damage.Fire/Ice/Lightning/Physical).
 *
 * The result is applied as a negative additive modifier to target Health.
 */
UCLASS()
class POF_API UARPGDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UARPGDamageExecution();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};

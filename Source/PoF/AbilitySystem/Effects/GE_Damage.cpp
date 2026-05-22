#include "AbilitySystem/Effects/GE_Damage.h"
#include "AbilitySystem/Effects/ARPGDamageExecution.h"

UGE_Damage::UGE_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Use the custom execution calculation for the full damage formula
	FGameplayEffectExecutionDefinition ExecDef;
	ExecDef.CalculationClass = UARPGDamageExecution::StaticClass();
	Executions.Add(ExecDef);
}

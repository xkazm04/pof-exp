#include "AbilitySystem/Effects/GE_DifficultyScaling.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"

UGE_DifficultyScaling::UGE_DifficultyScaling()
{
	// Infinite duration — stays active for the enemy's lifetime
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// --- MaxHealth: multiply by SetByCaller "Data.DifficultyMultiplier" ---
	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = UARPGAttributeSet::GetMaxHealthAttribute();
		Mod.ModifierOp = EGameplayModOp::MultiplyCompound;

		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = ARPGGameplayTags::Data_DifficultyMultiplier;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

		Modifiers.Add(Mod);
	}

	// --- Health: multiply by same value to keep current health proportional ---
	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = UARPGAttributeSet::GetHealthAttribute();
		Mod.ModifierOp = EGameplayModOp::MultiplyCompound;

		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = ARPGGameplayTags::Data_DifficultyMultiplier;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

		Modifiers.Add(Mod);
	}

	// --- AttackPower: multiply by same value ---
	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = UARPGAttributeSet::GetAttackPowerAttribute();
		Mod.ModifierOp = EGameplayModOp::MultiplyCompound;

		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = ARPGGameplayTags::Data_DifficultyMultiplier;
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);

		Modifiers.Add(Mod);
	}
}

#include "AbilitySystem/Effects/GE_InitAttributes.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"

UGE_InitAttributes::UGE_InitAttributes()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// Helper: add one SetByCaller modifier per attribute
	auto AddSetByCallerMod = [this](const FGameplayAttribute& Attribute, const FGameplayTag& Tag)
	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = Attribute;
		Mod.ModifierOp = EGameplayModOp::Override;

		FSetByCallerFloat SetByCaller;
		SetByCaller.DataTag = Tag;

		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		Modifiers.Add(Mod);
	};

	// Vitals
	AddSetByCallerMod(UARPGAttributeSet::GetHealthAttribute(),         ARPGGameplayTags::Data_Init_Health);
	AddSetByCallerMod(UARPGAttributeSet::GetMaxHealthAttribute(),      ARPGGameplayTags::Data_Init_MaxHealth);
	AddSetByCallerMod(UARPGAttributeSet::GetManaAttribute(),           ARPGGameplayTags::Data_Init_Mana);
	AddSetByCallerMod(UARPGAttributeSet::GetMaxManaAttribute(),        ARPGGameplayTags::Data_Init_MaxMana);

	// Primary Stats
	AddSetByCallerMod(UARPGAttributeSet::GetStrengthAttribute(),       ARPGGameplayTags::Data_Init_Strength);
	AddSetByCallerMod(UARPGAttributeSet::GetDexterityAttribute(),      ARPGGameplayTags::Data_Init_Dexterity);
	AddSetByCallerMod(UARPGAttributeSet::GetIntelligenceAttribute(),   ARPGGameplayTags::Data_Init_Intelligence);

	// Combat
	AddSetByCallerMod(UARPGAttributeSet::GetArmorAttribute(),          ARPGGameplayTags::Data_Init_Armor);
	AddSetByCallerMod(UARPGAttributeSet::GetAttackPowerAttribute(),    ARPGGameplayTags::Data_Init_AttackPower);
	AddSetByCallerMod(UARPGAttributeSet::GetCriticalChanceAttribute(), ARPGGameplayTags::Data_Init_CriticalChance);
	AddSetByCallerMod(UARPGAttributeSet::GetCriticalDamageAttribute(), ARPGGameplayTags::Data_Init_CriticalDamage);

	// Resistances
	AddSetByCallerMod(UARPGAttributeSet::GetFireResistanceAttribute(),      ARPGGameplayTags::Data_Init_FireResistance);
	AddSetByCallerMod(UARPGAttributeSet::GetIceResistanceAttribute(),       ARPGGameplayTags::Data_Init_IceResistance);
	AddSetByCallerMod(UARPGAttributeSet::GetLightningResistanceAttribute(), ARPGGameplayTags::Data_Init_LightningResistance);

	// Progression
	AddSetByCallerMod(UARPGAttributeSet::GetCurrentXPAttribute(),      ARPGGameplayTags::Data_Init_CurrentXP);
	AddSetByCallerMod(UARPGAttributeSet::GetXPToNextLevelAttribute(),  ARPGGameplayTags::Data_Init_XPToNextLevel);
	AddSetByCallerMod(UARPGAttributeSet::GetCharacterLevelAttribute(), ARPGGameplayTags::Data_Init_CharacterLevel);
}

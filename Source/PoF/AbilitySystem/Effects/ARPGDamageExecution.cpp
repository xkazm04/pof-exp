#include "AbilitySystem/Effects/ARPGDamageExecution.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystemComponent.h"

// Declare attribute capture definitions at file scope (GAS pattern)
struct FDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);
	DECLARE_ATTRIBUTE_CAPTUREDEF(FireResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IceResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(LightningResistance);

	FDamageStatics()
	{
		// Source captures (from the attacker)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UARPGAttributeSet, AttackPower,   Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UARPGAttributeSet, CriticalChance, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UARPGAttributeSet, CriticalDamage, Source, false);

		// Target captures (from the defender)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UARPGAttributeSet, Armor,              Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UARPGAttributeSet, FireResistance,     Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UARPGAttributeSet, IceResistance,      Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UARPGAttributeSet, LightningResistance, Target, false);
	}
};

static const FDamageStatics& DamageStatics()
{
	static FDamageStatics Statics;
	return Statics;
}

UARPGDamageExecution::UARPGDamageExecution()
{
	RelevantAttributesToCapture.Add(DamageStatics().AttackPowerDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalChanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalDamageDef);
	RelevantAttributesToCapture.Add(DamageStatics().ArmorDef);
	RelevantAttributesToCapture.Add(DamageStatics().FireResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().IceResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().LightningResistanceDef);
}

void UARPGDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// --- Invulnerability check ---
	const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	if (TargetASC && TargetASC->HasMatchingGameplayTag(ARPGGameplayTags::State_Invulnerable))
	{
		// Target is invulnerable — skip all damage
		return;
	}

	// --- Capture source attributes ---
	float AttackPower = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		DamageStatics().AttackPowerDef, FAggregatorEvaluateParameters(), AttackPower);

	float CriticalChance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		DamageStatics().CriticalChanceDef, FAggregatorEvaluateParameters(), CriticalChance);

	float CriticalDamage = 1.5f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		DamageStatics().CriticalDamageDef, FAggregatorEvaluateParameters(), CriticalDamage);

	// --- Capture target attributes ---
	float Armor = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		DamageStatics().ArmorDef, FAggregatorEvaluateParameters(), Armor);

	// --- Get SetByCaller values ---
	// BaseDamage: the raw damage number from the ability (required)
	const float BaseDamage = Spec.GetSetByCallerMagnitude(
		ARPGGameplayTags::Data_Damage_Base, /*WarnIfNotFound=*/ true, /*DefaultIfNotFound=*/ 0.f);

	// AttackPowerScaling: how much AttackPower contributes (optional, default 1.0)
	const float Scaling = Spec.GetSetByCallerMagnitude(
		ARPGGameplayTags::Data_Damage_Scaling, /*WarnIfNotFound=*/ false, /*DefaultIfNotFound=*/ 1.f);

	// --- Capture target elemental resistances ---
	float FireResistance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		DamageStatics().FireResistanceDef, FAggregatorEvaluateParameters(), FireResistance);

	float IceResistance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		DamageStatics().IceResistanceDef, FAggregatorEvaluateParameters(), IceResistance);

	float LightningResistance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		DamageStatics().LightningResistanceDef, FAggregatorEvaluateParameters(), LightningResistance);

	// --- Damage formula ---
	// Step 1: Raw damage = base + AttackPower * scaling
	const float RawDamage = BaseDamage + AttackPower * Scaling;

	// Step 2: Crit multiplier
	const bool bIsCrit = FMath::FRand() < CriticalChance;
	const float CritMultiplier = bIsCrit ? (1.f + CriticalDamage) : 1.f;

	// Step 3: Armor damage reduction (diminishing returns: Armor / (Armor + 100))
	const float ArmorReduction = Armor / (Armor + 100.f);

	// Step 4: Elemental resistance — look up from damage type tag on the GE spec
	float ElemResistance = 0.f;
	const FGameplayTagContainer& AssetTags = Spec.GetDynamicAssetTags();
	if (AssetTags.HasTag(ARPGGameplayTags::Damage_Fire))
	{
		ElemResistance = FMath::Clamp(FireResistance, 0.f, 0.9f);
	}
	else if (AssetTags.HasTag(ARPGGameplayTags::Damage_Ice))
	{
		ElemResistance = FMath::Clamp(IceResistance, 0.f, 0.9f);
	}
	else if (AssetTags.HasTag(ARPGGameplayTags::Damage_Lightning))
	{
		ElemResistance = FMath::Clamp(LightningResistance, 0.f, 0.9f);
	}

	// Step 5: Final damage
	float FinalDamage = RawDamage * CritMultiplier * (1.f - ArmorReduction) * (1.f - ElemResistance);
	FinalDamage = FMath::Max(FinalDamage, 0.f);

	// --- Output via meta attributes ---
	// IncomingCrit is written first so PostGameplayEffectExecute can read it
	// when the IncomingDamage modifier fires.
	if (FinalDamage > 0.f)
	{
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UARPGAttributeSet::GetIncomingCritAttribute(),
			EGameplayModOp::Override,
			bIsCrit ? 1.f : 0.f));

		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
			UARPGAttributeSet::GetIncomingDamageAttribute(),
			EGameplayModOp::Additive,
			FinalDamage));
	}
}

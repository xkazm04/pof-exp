#include "AbilitySystem/Effects/Generated/GE_Gen_Fireball_Burning.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Gen_Fireball_Burning::UGE_Gen_Fireball_Burning()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FScalableFloat DurationValue;
	DurationValue.Value = 3.0f;
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationValue);

	// Periodic application every 1.0s. This is the EFFECT period (DoT tick), NOT
	// the ability cooldown — ability cooldown is a separate cooldown GE.
	Period.Value = 1.0f;
	bExecutePeriodicEffectOnApplication = false;

	// Modifier: Health += -5 per tick
	FGameplayModifierInfo HealthMod;
	HealthMod.Attribute = UARPGAttributeSet::GetHealthAttribute();
	HealthMod.ModifierOp = EGameplayModOp::Additive;

	FScalableFloat HealthValue;
	HealthValue.Value = -5.f;
	HealthMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealthValue);

	Modifiers.Add(HealthMod);

	// Granted tag while active: State.Burning.
	// TODO(tag-delta): "State.Burning" is NOT yet declared in ARPGGameplayTags.h,
	// so RequestGameplayTag returns an invalid tag and the grant is skipped. Declare
	// it natively (see Generated/README.md) to activate this grant.
	const FGameplayTag BurningTag = FGameplayTag::RequestGameplayTag(FName("State.Burning"), /*ErrorIfNotFound=*/false);
	FInheritedTagContainer TagContainer;
	if (BurningTag.IsValid())
	{
		TagContainer.AddTag(BurningTag);
	}

	UTargetTagsGameplayEffectComponent* TargetTagsComp =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
	GEComponents.Add(TargetTagsComp);
	TargetTagsComp->SetAndApplyTargetTagChanges(TagContainer);
}

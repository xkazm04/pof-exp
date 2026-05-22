#include "AbilitySystem/ARPGGameplayAbility.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/ARPGCharacterBase.h"

UARPGGameplayAbility::UARPGGameplayAbility()
{
	// Default instancing: one instance per actor. Allows the ability to store
	// per-activation state (e.g. combo count, charge time).
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Block activation while dead or stunned
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(ARPGGameplayTags::State_Stunned);
}

AARPGCharacterBase* UARPGGameplayAbility::GetARPGCharacter() const
{
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	return Info ? Cast<AARPGCharacterBase>(Info->AvatarActor.Get()) : nullptr;
}

UAbilitySystemComponent* UARPGGameplayAbility::GetARPGAbilitySystemComponent() const
{
	const FGameplayAbilityActorInfo* Info = GetCurrentActorInfo();
	return Info ? Info->AbilitySystemComponent.Get() : nullptr;
}

const UARPGAttributeSet* UARPGGameplayAbility::GetARPGAttributeSet() const
{
	UAbilitySystemComponent* ASC = GetARPGAbilitySystemComponent();
	return ASC ? ASC->GetSet<UARPGAttributeSet>() : nullptr;
}

FActiveGameplayEffectHandle UARPGGameplayAbility::ApplyEffectToTarget(
	UAbilitySystemComponent* TargetASC,
	TSubclassOf<UGameplayEffect> EffectClass,
	float Level)
{
	if (!TargetASC || !EffectClass)
	{
		return FActiveGameplayEffectHandle();
	}

	UAbilitySystemComponent* SourceASC = GetARPGAbilitySystemComponent();
	if (!SourceASC)
	{
		return FActiveGameplayEffectHandle();
	}

	const float EffectLevel = (Level < 0.f) ? GetAbilityLevel() : Level;

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(this);

	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, EffectLevel, Context);
	if (!Spec.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	return SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
}

FActiveGameplayEffectHandle UARPGGameplayAbility::ApplyEffectToSelf(
	TSubclassOf<UGameplayEffect> EffectClass,
	float Level)
{
	return ApplyEffectToTarget(GetARPGAbilitySystemComponent(), EffectClass, Level);
}

bool UARPGGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	if (AbilityManaCost > 0.f)
	{
		const UARPGAttributeSet* Attrs = GetARPGAttributeSet();
		if (!Attrs || Attrs->GetMana() < AbilityManaCost)
		{
			return false;
		}
	}

	return true;
}

void UARPGGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

	if (AbilityManaCost > 0.f)
	{
		UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
		if (ASC)
		{
			// Directly modify the Mana attribute
			ASC->ApplyModToAttribute(UARPGAttributeSet::GetManaAttribute(), EGameplayModOp::Additive, -AbilityManaCost);
		}
	}
}

void UARPGGameplayAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// CommitAbility checks cost and applies cooldown in one call.
	// If it fails (not enough resources, cooldown active), cancel.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (bAutoEndAbility)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
	}
}

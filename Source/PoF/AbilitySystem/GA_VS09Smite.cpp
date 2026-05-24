#include "AbilitySystem/GA_VS09Smite.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "AbilitySystem/Effects/GE_Damage.h"
#include "Character/ARPGCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Engine/OverlapResult.h"

UGA_VS09Smite::UGA_VS09Smite()
{
	bAutoEndAbility = false;
	AbilityManaCost = 0.f; // no cost — CommitAbility always succeeds for the gray-box proof

	// Default the damage effect so the raw C++ ability can be granted directly
	// (no config-BP). A Blueprint subclass may still override this.
	DamageEffect = UGE_Damage::StaticClass();
}

void UGA_VS09Smite::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PerformRadialDamage();

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGA_VS09Smite::PerformRadialDamage()
{
	AARPGCharacterBase* Character = GetARPGCharacter();
	if (!Character || !DamageEffect) return;

	UAbilitySystemComponent* SourceASC = GetARPGAbilitySystemComponent();
	if (!SourceASC) return;

	const FVector Origin = Character->GetActorLocation();
	const FVector Forward = Character->GetActorForwardVector();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Character);

	Character->GetWorld()->OverlapMultiByChannel(
		Overlaps,
		Origin,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(HitRadius),
		QueryParams);

	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(HitHalfAngle));

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor) continue;

		const FVector ToTarget = (HitActor->GetActorLocation() - Origin).GetSafeNormal();
		if (FVector::DotProduct(Forward, ToTarget) < CosHalfAngle)
		{
			continue;
		}

		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(HitActor);
		UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC) continue;

		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffect, GetAbilityLevel(), Context);
		if (!SpecHandle.IsValid()) continue;

		SpecHandle.Data->SetSetByCallerMagnitude(ARPGGameplayTags::Data_Damage_Base, BaseDamage);
		SpecHandle.Data->AddDynamicAssetTag(ARPGGameplayTags::Damage_Physical);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

		UE_LOG(LogTemp, Log, TEXT("[GA_VS09Smite] Hit %s for %.1f base damage"), *HitActor->GetName(), BaseDamage);
	}
}

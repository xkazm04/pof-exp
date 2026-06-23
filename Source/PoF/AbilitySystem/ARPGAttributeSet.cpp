#include "AbilitySystem/ARPGAttributeSet.h"
#include "AbilitySystem/ARPGGameplayTags.h"
#include "Player/ARPGPlayerCharacter.h"
#include "Character/ARPGCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GameplayCueManager.h"

namespace
{
	/** Kick the damaged character's upper body away from the attacker (physics flinch). */
	void TriggerPhysicsFlinch(AActor* InstigatorActor, AActor* TargetActor)
	{
		AARPGCharacterBase* HitChar = Cast<AARPGCharacterBase>(TargetActor);
		if (!HitChar) return;
		FVector HitDir = -HitChar->GetActorForwardVector(); // fallback: shoved backward
		if (InstigatorActor)
		{
			const FVector Delta = HitChar->GetActorLocation() - InstigatorActor->GetActorLocation();
			if (!Delta.IsNearlyZero()) { HitDir = Delta.GetSafeNormal(); }
		}
		HitChar->PhysicalHitReaction(HitDir, 1.0f);
	}
}

FOnDamageNumberRequestedNative UARPGAttributeSet::OnDamageNumberRequestedGlobal;

UARPGAttributeSet::UARPGAttributeSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitMana(50.f);
	InitMaxMana(50.f);
	InitStrength(10.f);
	InitDexterity(10.f);
	InitIntelligence(10.f);
	InitArmor(0.f);
	InitAttackPower(10.f);
	InitCriticalChance(0.05f);
	InitCriticalDamage(1.5f);

	// Resistances (0.0 = no resistance)
	InitFireResistance(0.f);
	InitIceResistance(0.f);
	InitLightningResistance(0.f);

	// Progression
	InitCurrentXP(0.f);
	InitXPToNextLevel(100.f);
	InitCharacterLevel(1.f);

	// Meta attributes default to 0
	InitIncomingDamage(0.f);
	InitIncomingCrit(0.f);
	InitIncomingHeal(0.f);
	InitIncomingXP(0.f);
}

void UARPGAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Clamp vitals to their max counterparts
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
	}
	else if (Attribute == GetMaxManaAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	// Clamp stats that shouldn't go negative
	else if (Attribute == GetArmorAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetAttackPowerAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetCriticalChanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 1.f);
	}
	else if (Attribute == GetCriticalDamageAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	// Clamp resistances to [0, 0.9] — 90% cap prevents full immunity
	else if (Attribute == GetFireResistanceAttribute()
		|| Attribute == GetIceResistanceAttribute()
		|| Attribute == GetLightningResistanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 0.9f);
	}
	// Clamp progression attributes
	else if (Attribute == GetCurrentXPAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetXPToNextLevelAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetCharacterLevelAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 1.f, 50.f);
	}
}

void UARPGAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// =====================================================================
	// Meta attribute: IncomingDamage (set by damage execution)
	// =====================================================================
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float DamageAmount = GetIncomingDamage();
		const bool bIsCrit = GetIncomingCrit() > 0.5f;

		// Consume meta attributes
		SetIncomingDamage(0.f);
		SetIncomingCrit(0.f);

		if (DamageAmount > 0.f)
		{
			// Apply to Health
			const float OldHealth = GetHealth();
			SetHealth(FMath::Clamp(OldHealth - DamageAmount, 0.f, GetMaxHealth()));

			AActor* Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();
			UAbilitySystemComponent* OwnerASC = GetOwningAbilitySystemComponent();
			AActor* TargetActor = OwnerASC ? OwnerASC->GetAvatarActor() : nullptr;

			// Determine hit location for floating number
			FVector HitLocation = TargetActor ? TargetActor->GetActorLocation() : FVector::ZeroVector;
			const FHitResult* HitResult = Data.EffectSpec.GetEffectContext().GetHitResult();
			if (HitResult && HitResult->bBlockingHit)
			{
				HitLocation = HitResult->ImpactPoint;
			}

			// Determine damage type from the effect spec's dynamic asset tags
			FGameplayTag DamageTypeTag;
			const FGameplayTagContainer& AssetTags = Data.EffectSpec.GetDynamicAssetTags();
			if (AssetTags.HasTag(ARPGGameplayTags::Damage_Fire))
			{
				DamageTypeTag = ARPGGameplayTags::Damage_Fire;
			}
			else if (AssetTags.HasTag(ARPGGameplayTags::Damage_Ice))
			{
				DamageTypeTag = ARPGGameplayTags::Damage_Ice;
			}
			else if (AssetTags.HasTag(ARPGGameplayTags::Damage_Lightning))
			{
				DamageTypeTag = ARPGGameplayTags::Damage_Lightning;
			}
			else
			{
				DamageTypeTag = ARPGGameplayTags::Damage_Physical;
			}

			// Broadcast damage number (per-instance + global)
			OnDamageNumberRequested.Broadcast(
				TargetActor, DamageAmount, bIsCrit, false, DamageTypeTag, HitLocation);
			OnDamageNumberRequestedGlobal.Broadcast(
				TargetActor, DamageAmount, bIsCrit, false, DamageTypeTag, HitLocation);

			// Execute GameplayCue for hit impact VFX/SFX
			if (OwnerASC)
			{
				FGameplayTag CueTag;
				if (DamageTypeTag == ARPGGameplayTags::Damage_Fire)
				{
					CueTag = ARPGGameplayTags::GameplayCue_HitImpact_Fire;
				}
				else if (DamageTypeTag == ARPGGameplayTags::Damage_Ice)
				{
					CueTag = ARPGGameplayTags::GameplayCue_HitImpact_Ice;
				}
				else if (DamageTypeTag == ARPGGameplayTags::Damage_Lightning)
				{
					CueTag = ARPGGameplayTags::GameplayCue_HitImpact_Lightning;
				}
				else
				{
					CueTag = ARPGGameplayTags::GameplayCue_HitImpact;
				}

				FGameplayCueParameters CueParams;
				CueParams.Location = HitLocation;
				CueParams.RawMagnitude = DamageAmount;
				OwnerASC->ExecuteGameplayCue(CueTag, CueParams);
			}

			// Death / Hit React events
			if (GetHealth() <= 0.f)
			{
				const bool bAlreadyDead = OwnerASC && OwnerASC->HasMatchingGameplayTag(ARPGGameplayTags::State_Dead);

				if (!bAlreadyDead)
				{
					OnHealthDepleted.Broadcast(Instigator);

					if (OwnerASC)
					{
						FGameplayEventData DeathPayload;
						DeathPayload.EventTag = ARPGGameplayTags::Event_Death;
						DeathPayload.Instigator = Instigator;
						DeathPayload.Target = TargetActor;
						DeathPayload.EventMagnitude = -DamageAmount;
						OwnerASC->HandleGameplayEvent(ARPGGameplayTags::Event_Death, &DeathPayload);
					}
				}
			}
			else
			{
				if (OwnerASC)
				{
					FGameplayEventData HitReactPayload;
					HitReactPayload.EventTag = ARPGGameplayTags::Event_HitReact;
					HitReactPayload.Instigator = Instigator;
					HitReactPayload.Target = TargetActor;
					HitReactPayload.EventMagnitude = -DamageAmount;
					OwnerASC->HandleGameplayEvent(ARPGGameplayTags::Event_HitReact, &HitReactPayload);
				}
				// Physics flinch: kick the upper body away from the attacker (impact weight the
				// absent/code-FK hit-react montage can't provide).
				TriggerPhysicsFlinch(Instigator, TargetActor);
			}
		}

		return;
	}

	// =====================================================================
	// Meta attribute: IncomingHeal
	// =====================================================================
	if (Data.EvaluatedData.Attribute == GetIncomingHealAttribute())
	{
		const float HealAmount = GetIncomingHeal();
		SetIncomingHeal(0.f);

		if (HealAmount > 0.f)
		{
			const float OldHealth = GetHealth();
			SetHealth(FMath::Clamp(OldHealth + HealAmount, 0.f, GetMaxHealth()));
			const float ActualHeal = GetHealth() - OldHealth;

			if (ActualHeal > 0.f)
			{
				UAbilitySystemComponent* OwnerASC = GetOwningAbilitySystemComponent();
				AActor* TargetActor = OwnerASC ? OwnerASC->GetAvatarActor() : nullptr;
				FVector Location = TargetActor ? TargetActor->GetActorLocation() : FVector::ZeroVector;

				OnDamageNumberRequested.Broadcast(
					TargetActor, ActualHeal, false, true, FGameplayTag(), Location);
				OnDamageNumberRequestedGlobal.Broadcast(
					TargetActor, ActualHeal, false, true, FGameplayTag(), Location);

				// Execute heal GameplayCue
				if (OwnerASC)
				{
					FGameplayCueParameters CueParams;
					CueParams.Location = Location;
					CueParams.RawMagnitude = ActualHeal;
					OwnerASC->ExecuteGameplayCue(ARPGGameplayTags::GameplayCue_Heal, CueParams);
				}
			}
		}

		return;
	}

	// =====================================================================
	// Direct Health modification (from non-execution effects like GE_Heal)
	// =====================================================================
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));

		// Positive direct health changes = heal (for backward compat with GE_Heal)
		if (Data.EvaluatedData.Magnitude > 0.f)
		{
			UAbilitySystemComponent* OwnerASC = GetOwningAbilitySystemComponent();
			AActor* TargetActor = OwnerASC ? OwnerASC->GetAvatarActor() : nullptr;
			FVector Location = TargetActor ? TargetActor->GetActorLocation() : FVector::ZeroVector;

			OnDamageNumberRequested.Broadcast(
				TargetActor, Data.EvaluatedData.Magnitude, false, true, FGameplayTag(), Location);
			OnDamageNumberRequestedGlobal.Broadcast(
				TargetActor, Data.EvaluatedData.Magnitude, false, true, FGameplayTag(), Location);
		}
		// Negative direct health changes = damage (backward compat for any direct-to-health effects)
		else if (Data.EvaluatedData.Magnitude < 0.f)
		{
			AActor* Instigator = Data.EffectSpec.GetEffectContext().GetInstigator();
			UAbilitySystemComponent* OwnerASC = GetOwningAbilitySystemComponent();
			AActor* TargetActor = OwnerASC ? OwnerASC->GetAvatarActor() : nullptr;
			FVector HitLocation = TargetActor ? TargetActor->GetActorLocation() : FVector::ZeroVector;
			const FHitResult* HitResult = Data.EffectSpec.GetEffectContext().GetHitResult();
			if (HitResult && HitResult->bBlockingHit)
			{
				HitLocation = HitResult->ImpactPoint;
			}

			OnDamageNumberRequested.Broadcast(
				TargetActor, FMath::Abs(Data.EvaluatedData.Magnitude), false, false,
				ARPGGameplayTags::Damage_Physical, HitLocation);
			OnDamageNumberRequestedGlobal.Broadcast(
				TargetActor, FMath::Abs(Data.EvaluatedData.Magnitude), false, false,
				ARPGGameplayTags::Damage_Physical, HitLocation);

			if (GetHealth() <= 0.f)
			{
				const bool bAlreadyDead = OwnerASC && OwnerASC->HasMatchingGameplayTag(ARPGGameplayTags::State_Dead);
				if (!bAlreadyDead)
				{
					OnHealthDepleted.Broadcast(Instigator);
					if (OwnerASC)
					{
						FGameplayEventData DeathPayload;
						DeathPayload.EventTag = ARPGGameplayTags::Event_Death;
						DeathPayload.Instigator = Instigator;
						DeathPayload.Target = TargetActor;
						DeathPayload.EventMagnitude = Data.EvaluatedData.Magnitude;
						OwnerASC->HandleGameplayEvent(ARPGGameplayTags::Event_Death, &DeathPayload);
					}
				}
			}
			else
			{
				if (OwnerASC)
				{
					FGameplayEventData HitReactPayload;
					HitReactPayload.EventTag = ARPGGameplayTags::Event_HitReact;
					HitReactPayload.Instigator = Instigator;
					HitReactPayload.Target = TargetActor;
					HitReactPayload.EventMagnitude = Data.EvaluatedData.Magnitude;
					OwnerASC->HandleGameplayEvent(ARPGGameplayTags::Event_HitReact, &HitReactPayload);
				}
				TriggerPhysicsFlinch(Instigator, TargetActor);
			}
		}
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}

	// =====================================================================
	// Meta attribute: IncomingXP (set by GE_AwardXP)
	// =====================================================================
	if (Data.EvaluatedData.Attribute == GetIncomingXPAttribute())
	{
		const float XPAmount = GetIncomingXP();
		SetIncomingXP(0.f);

		if (XPAmount > 0.f)
		{
			UAbilitySystemComponent* OwnerASC = GetOwningAbilitySystemComponent();
			AActor* TargetActor = OwnerASC ? OwnerASC->GetAvatarActor() : nullptr;

			if (AARPGPlayerCharacter* Player = Cast<AARPGPlayerCharacter>(TargetActor))
			{
				Player->AddExperience(XPAmount);
			}
		}
	}
}

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ARPGAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthDepleted, AActor*, KillingActor);

/**
 * Broadcast when a damage or heal number should be displayed.
 * @param TargetActor  The actor whose health changed.
 * @param DamageAmount Absolute value of the damage or heal.
 * @param bIsCrit      Whether this was a critical hit.
 * @param bIsHeal      True for heals, false for damage.
 * @param DamageType   Gameplay tag for damage element (Damage.Physical, etc.). Empty for heals.
 * @param HitLocation  World-space location for the floating number.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(
	FOnDamageNumberRequested,
	AActor*, TargetActor,
	float, DamageAmount,
	bool, bIsCrit,
	bool, bIsHeal,
	FGameplayTag, DamageType,
	FVector, HitLocation);

/** Native static delegate for global damage number events. No per-instance binding needed. */
DECLARE_MULTICAST_DELEGATE_SixParams(
	FOnDamageNumberRequestedNative,
	AActor* /*TargetActor*/,
	float   /*DamageAmount*/,
	bool    /*bIsCrit*/,
	bool    /*bIsHeal*/,
	FGameplayTag /*DamageType*/,
	FVector /*HitLocation*/);

UCLASS()
class POF_API UARPGAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UARPGAttributeSet();

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	/** Global static delegate — any attribute set instance broadcasts here. */
	static FOnDamageNumberRequestedNative OnDamageNumberRequestedGlobal;

	// --- Meta Attributes (transient, consumed in PostGameplayEffectExecute) ---

	/** Incoming damage before being applied to Health. Set by the damage execution. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, IncomingDamage)

	/** 1.0 if the incoming damage was a critical hit, 0.0 otherwise. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Meta")
	FGameplayAttributeData IncomingCrit;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, IncomingCrit)

	/** Incoming heal before being applied to Health. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Meta")
	FGameplayAttributeData IncomingHeal;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, IncomingHeal)

	/** Incoming XP before being applied to CurrentXP. Consumed in PostGameplayEffectExecute. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Meta")
	FGameplayAttributeData IncomingXP;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, IncomingXP)

	// --- Vitals ---

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vitals")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vitals")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vitals")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, Mana)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Vitals")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, MaxMana)

	// --- Primary Stats ---

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Primary")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, Strength)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Primary")
	FGameplayAttributeData Dexterity;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, Dexterity)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Primary")
	FGameplayAttributeData Intelligence;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, Intelligence)

	// --- Combat ---

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, Armor)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, AttackPower)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData CriticalChance;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, CriticalChance)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData CriticalDamage;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, CriticalDamage)

	// --- Elemental Resistances (0.0 = no resistance, 1.0 = immune) ---

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Resistance")
	FGameplayAttributeData FireResistance;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, FireResistance)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Resistance")
	FGameplayAttributeData IceResistance;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, IceResistance)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Resistance")
	FGameplayAttributeData LightningResistance;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, LightningResistance)

	// --- Progression ---

	/** Accumulated XP toward the next level. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Progression")
	FGameplayAttributeData CurrentXP;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, CurrentXP)

	/** XP required to reach the next level (looked up from XP curve table). */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Progression")
	FGameplayAttributeData XPToNextLevel;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, XPToNextLevel)

	/** Current character level (stored as float, treated as int). */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Progression")
	FGameplayAttributeData CharacterLevel;
	ATTRIBUTE_ACCESSORS(UARPGAttributeSet, CharacterLevel)

	// --- Death delegate ---

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnHealthDepleted OnHealthDepleted;

	/** Broadcast when a floating damage/heal number should be displayed. */
	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnDamageNumberRequested OnDamageNumberRequested;
};

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ARPGReplicationTypes.generated.h"

/**
 * Compact replicated snapshot of a character's vital stats.
 * Sent via multicast RPC or property replication to keep clients in sync.
 */
USTRUCT(BlueprintType)
struct FARPGReplicatedVitals
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float Health = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float MaxHealth = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float Mana = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float MaxMana = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float Stamina = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float MaxStamina = 0.f;

	UPROPERTY(BlueprintReadOnly)
	int32 CharacterLevel = 1;
};

/**
 * Replicated combat event payload — damage, heals, status effects.
 * Sent via multicast so all clients can display feedback.
 */
USTRUCT(BlueprintType)
struct FARPGReplicatedCombatEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AActor> Instigator;

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AActor> Target;

	UPROPERTY(BlueprintReadOnly)
	float Amount = 0.f;

	UPROPERTY(BlueprintReadOnly)
	bool bIsCrit = false;

	UPROPERTY(BlueprintReadOnly)
	bool bIsHeal = false;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag DamageType;

	UPROPERTY(BlueprintReadOnly)
	FVector HitLocation = FVector::ZeroVector;
};

/**
 * Replicated ability activation payload for cosmetic replication.
 */
USTRUCT(BlueprintType)
struct FARPGReplicatedAbilityEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag AbilityTag;

	UPROPERTY(BlueprintReadOnly)
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AActor> TargetActor;

	/** Montage section index for combo replication. */
	UPROPERTY(BlueprintReadOnly)
	int32 MontageSection = 0;
};

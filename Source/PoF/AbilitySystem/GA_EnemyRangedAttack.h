#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "GA_EnemyRangedAttack.generated.h"

class UAnimMontage;
class UGameplayEffect;
class AARPGProjectile;

/**
 * Enemy ranged attack ability.
 *
 * Flow:
 *   1. Plays a cast/throw montage
 *   2. At the release point (Event.MeleeHit reused as generic attack event),
 *      spawns an AARPGProjectile aimed at the current target
 *   3. The projectile handles its own collision and damage application
 *   4. Ability ends when the montage completes
 *
 * The projectile class, speed, and damage are all configurable.
 */
UCLASS()
class POF_API UGA_EnemyRangedAttack : public UARPGGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_EnemyRangedAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** The cast/throw montage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Ranged")
	TObjectPtr<UAnimMontage> CastMontage;

	/** Play rate for the montage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Ranged", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MontagePlayRate = 1.0f;

	/** Projectile class to spawn. Must be AARPGProjectile or subclass. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Ranged|Projectile")
	TSubclassOf<AARPGProjectile> ProjectileClass;

	/** Spawn offset from the character origin (local space). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Ranged|Projectile")
	FVector SpawnOffset = FVector(100.f, 0.f, 80.f);

	/** Damage GE class (should be GE_Damage or subclass). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Ranged|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** Base damage for the projectile. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Ranged|Damage")
	float BaseDamage = 25.f;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnReleaseEvent(FGameplayEventData Payload);

	void SpawnProjectile();
};

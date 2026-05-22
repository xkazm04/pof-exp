#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "GA_Fireball.generated.h"

class UAnimMontage;
class UGameplayEffect;
class AARPGProjectile;
class UNiagaraSystem;

/**
 * Fireball ability — ranged projectile that explodes on impact for AoE fire damage.
 *
 * Flow:
 *   1. CommitAbility (mana cost + cooldown)
 *   2. Enable cursor aim so the player faces the cursor
 *   3. Play cast montage
 *   4. On Event.MeleeHit (reused as "release" event from anim notify):
 *      spawn projectile toward cursor world location
 *   5. Projectile handles AoE explosion on hit (via modified AARPGProjectile)
 *   6. End ability when montage finishes
 *
 * The projectile explodes on impact dealing AoE fire damage within ExplosionRadius.
 */
UCLASS()
class POF_API UGA_Fireball : public UARPGGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Fireball();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fireball")
	TObjectPtr<UAnimMontage> CastMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fireball", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fireball|Projectile")
	TSubclassOf<AARPGProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fireball|Projectile")
	FVector SpawnOffset = FVector(80.f, 0.f, 60.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fireball|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fireball|Damage")
	float BaseDamage = 35.f;

	/** AoE explosion radius at the impact point. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fireball|AoE", meta = (ClampMin = "50"))
	float ExplosionRadius = 250.f;

	/** Placeholder VFX for cast. Spawned at hand on release. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fireball|VFX")
	TObjectPtr<UNiagaraSystem> CastVFX;

	/** Placeholder VFX for explosion at impact. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fireball|VFX")
	TObjectPtr<UNiagaraSystem> ExplosionVFX;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnReleaseEvent(FGameplayEventData Payload);

	void SpawnProjectile();
};

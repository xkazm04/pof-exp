#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "ARPGProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class UGameplayEffect;
class UAbilitySystemComponent;

/**
 * Base projectile actor for ranged attacks.
 *
 * Features:
 * - USphereComponent for collision detection
 * - UProjectileMovementComponent for movement (speed, gravity, homing, bounce)
 * - Optional UNiagaraComponent for trail VFX
 * - Impact VFX/SFX with surface-aware material lookup
 * - Optional area-of-effect damage on impact
 * - Applies a GE_Damage on hit via the instigator's ASC
 * - Auto-destroys after MaxLifeTime or on hit
 */
UCLASS()
class POF_API AARPGProjectile : public AActor
{
	GENERATED_BODY()

public:
	AARPGProjectile();

	/**
	 * Initialize the projectile with damage parameters after spawning.
	 * Must be called before the projectile hits anything.
	 */
	void InitDamage(UAbilitySystemComponent* InSourceASC, TSubclassOf<UGameplayEffect> InDamageEffect, float InBaseDamage, float InLevel);

	/**
	 * Add a damage type tag (e.g., Damage.Fire) that will be applied as a
	 * dynamic asset tag on the GE spec. Used by the damage execution to
	 * select elemental resistance and by PostGameplayEffectExecute for VFX.
	 */
	void AddDamageTag(FGameplayTag DamageTypeTag) { DamageTags.AddTag(DamageTypeTag); }

	/**
	 * Enable homing on a specific actor's root component.
	 * Call after spawning the projectile.
	 */
	UFUNCTION(BlueprintCallable, Category = "Projectile|Homing")
	void EnableHoming(AActor* HomingTarget);

	/** Get the projectile movement component. */
	UFUNCTION(BlueprintPure, Category = "Projectile")
	UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|VFX")
	TObjectPtr<UNiagaraComponent> VFXComponent;

	// --- Speed & Movement ---

	/** Projectile speed (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "100"))
	float Speed = 2000.f;

	/** Collision sphere radius. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "5"))
	float SphereRadius = 20.f;

	/** Auto-destroy after this many seconds if no hit occurs. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.5"))
	float MaxLifeTime = 5.f;

	/** Gravity scale (0 = no gravity, 1 = normal gravity). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Movement")
	float GravityScale = 0.f;

	// --- Bouncing ---

	/** Whether the projectile bounces off surfaces. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Bounce")
	bool bShouldBounce = false;

	/** Bounciness factor (0 = no bounce, 1 = full energy preservation). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Bounce", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "bShouldBounce"))
	float Bounciness = 0.6f;

	/** Max number of bounces before the projectile is destroyed. 0 = unlimited. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Bounce", meta = (ClampMin = "0", EditCondition = "bShouldBounce"))
	int32 MaxBounces = 3;

	// --- Homing ---

	/** Homing acceleration magnitude when homing is enabled. Higher = tighter tracking. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Homing", meta = (ClampMin = "0"))
	float HomingAcceleration = 5000.f;

	// --- Area Damage ---

	/** Whether to deal area-of-effect damage on impact. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|AoE")
	bool bAreaDamage = false;

	/** Radius of the area-of-effect damage sphere. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|AoE", meta = (ClampMin = "50", EditCondition = "bAreaDamage"))
	float AreaDamageRadius = 200.f;

	/** Damage falloff at the edge of the AoE (1 = full damage everywhere, 0 = zero at edge). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|AoE", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "bAreaDamage"))
	float AreaDamageFalloff = 0.5f;

	// --- Impact FX ---

	/** Niagara system spawned at the impact point. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Impact")
	TObjectPtr<UNiagaraSystem> ImpactVFX;

	/** Sound played on impact. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Impact")
	TObjectPtr<USoundBase> ImpactSound;

	virtual void BeginPlay() override;

	/** Called on impact — can be overridden for custom behavior. */
	virtual void OnImpact(AActor* OtherActor, const FHitResult& HitResult);

private:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	void ApplyDamageToTarget(AActor* TargetActor, const FHitResult& HitResult);
	void ApplyAreaDamage(const FVector& Origin);
	void SpawnImpactEffects(const FVector& Location, const FVector& Normal);

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> SourceASC;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	float DamageBaseMagnitude = 0.f;
	float EffectLevel = 1.f;
	int32 BounceCount = 0;

	/** Damage type tags applied to the GE spec (e.g., Damage.Fire). */
	FGameplayTagContainer DamageTags;
};

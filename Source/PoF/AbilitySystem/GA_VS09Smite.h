#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "GA_VS09Smite.generated.h"

class UGameplayEffect;

/**
 * Folder-09 generator proof ability — a deterministic gray-box GameplayAbility.
 *
 * On activation it commits and applies a single synchronous radial GE_Damage to
 * every ASC-bearing pawn within HitRadius (no montage, no projectile, no binary
 * config), then ends. This lets a headless functional test verify the
 * generated-ability lifecycle's `verified` gate without flaky animation/physics —
 * the same gray-box pattern GA_EnemyMeleeAttack uses for its fallback window,
 * reduced to the most deterministic form for the Round-1 live proof.
 */
UCLASS()
class POF_API UGA_VS09Smite : public UARPGGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_VS09Smite();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** Damage GE class (defaults to GE_Damage so the raw C++ ability is grantable directly). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VS09Smite|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	/** Base damage. Passed as SetByCaller Data.Damage.Base. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VS09Smite|Damage")
	float BaseDamage = 25.f;

	/** Radius of the radial overlap check (from caster origin). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VS09Smite|HitDetection", meta = (ClampMin = "50"))
	float HitRadius = 300.f;

	/** Half-angle in degrees (180 = omnidirectional — facing-independent for a deterministic gray-box). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VS09Smite|HitDetection", meta = (ClampMin = "10", ClampMax = "180"))
	float HitHalfAngle = 180.f;

private:
	void PerformRadialDamage();
};

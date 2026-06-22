#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/ARPGGameplayAbility.h"
#include "GA_ForcePush.generated.h"

class UGameplayEffect;
class UNiagaraSystem;

/**
 * Force Push — a Star Wars-style telekinetic shove (player ability, hotbar slot 1 / key '1').
 *
 * Fire-and-forget. On activation it finds pawns inside a forward cone and:
 *   1. launches each target away from the caster (LaunchCharacter physics knockback), and
 *   2. applies a light physical-damage GameplayEffect (UGE_Damage via SetByCaller).
 *
 * Patterned on UGA_GroundSlam, but DIRECTIONAL (a forward cone rather than a full AoE)
 * with knockback as the signature effect instead of a stun. Commits mana + cooldown.
 */
UCLASS()
class POF_API UGA_ForcePush : public UARPGGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ForcePush();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	/** Light physical damage applied to each pushed target. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ForcePush|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ForcePush|Damage", meta = (ClampMin = "0"))
	float BaseDamage = 15.f;

	/** Reach of the push cone (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ForcePush|Targeting", meta = (ClampMin = "100"))
	float PushRange = 600.f;

	/** Half-angle of the forward cone (degrees). Targets outside the cone are unaffected. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ForcePush|Targeting", meta = (ClampMin = "5", ClampMax = "180"))
	float ConeHalfAngleDeg = 55.f;

	/** Horizontal launch speed imparted to each target (cm/s). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ForcePush|Knockback", meta = (ClampMin = "0"))
	float HorizontalKnockback = 1100.f;

	/** Vertical launch speed imparted to each target (cm/s) — gives the shove an upward arc. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ForcePush|Knockback", meta = (ClampMin = "0"))
	float VerticalKnockback = 350.f;

	/** Optional impact VFX spawned at the caster on activation. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ForcePush|VFX")
	TObjectPtr<UNiagaraSystem> PushVFX;

private:
	/** Find valid targets in the forward cone and apply knockback + damage. */
	void ApplyForcePush();
};

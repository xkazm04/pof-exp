#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "CombatFeedbackComponent.generated.h"

class UCameraShakeBase;
class UNiagaraSystem;
class USoundBase;

/**
 * Handles combat feedback effects: camera shake, hitstop, hit VFX, hit SFX.
 * Attach to the PlayerController. Subscribes to the global static delegate
 * on UARPGAttributeSet so all damage events trigger feedback.
 */
UCLASS(ClassGroup = "Combat", meta = (BlueprintSpawnableComponent))
class POF_API UCombatFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatFeedbackComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// =====================================================================
	// Camera Shake
	// =====================================================================

	/** Camera shake class for normal hits. Assign a BP-configured CameraShake asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|CameraShake")
	TSubclassOf<UCameraShakeBase> LightHitShake;

	/** Camera shake class for critical hits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|CameraShake")
	TSubclassOf<UCameraShakeBase> HeavyHitShake;

	/** Scale multiplier for the camera shake. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|CameraShake", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float CameraShakeScale = 1.0f;

	// =====================================================================
	// Hitstop
	// =====================================================================

	/** Whether to apply hitstop (brief time dilation) on hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|Hitstop")
	bool bEnableHitstop = true;

	/** CustomTimeDilation value during hitstop (0.01 = near freeze). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|Hitstop", meta = (ClampMin = "0.001", ClampMax = "1.0"))
	float HitstopTimeDilation = 0.01f;

	/** Duration of the hitstop in real seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|Hitstop", meta = (ClampMin = "0.01", ClampMax = "0.5"))
	float HitstopDuration = 0.05f;

	// =====================================================================
	// Hit VFX
	// =====================================================================

	/** Niagara system to spawn at the impact point on hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|VFX")
	TObjectPtr<UNiagaraSystem> HitImpactVFX;

	/** Scale of the hit impact VFX. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|VFX")
	FVector HitVFXScale = FVector(1.0f);

	// =====================================================================
	// Hit SFX
	// =====================================================================

	/** Default hit sound (physical/generic). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|SFX")
	TObjectPtr<USoundBase> HitSound_Physical;

	/** Hit sound for fire damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|SFX")
	TObjectPtr<USoundBase> HitSound_Fire;

	/** Hit sound for ice damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|SFX")
	TObjectPtr<USoundBase> HitSound_Ice;

	/** Hit sound for lightning damage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|SFX")
	TObjectPtr<USoundBase> HitSound_Lightning;

	/** Hit sound for critical hits (played in addition to the element sound). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|SFX")
	TObjectPtr<USoundBase> HitSound_Crit;

	/** Volume multiplier for hit sounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|SFX", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float HitSoundVolume = 1.0f;

	// =====================================================================
	// Controller Vibration (Force Feedback)
	// =====================================================================

	/** Whether to trigger controller vibration on hit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|Vibration")
	bool bEnableControllerVibration = true;

	/** Vibration intensity for normal hits [0..1]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|Vibration", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableControllerVibration"))
	float NormalHitVibrationIntensity = 0.3f;

	/** Vibration intensity for critical hits [0..1]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|Vibration", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnableControllerVibration"))
	float CritHitVibrationIntensity = 0.7f;

	/** Duration of the controller vibration in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatFeedback|Vibration", meta = (ClampMin = "0.01", ClampMax = "1.0", EditCondition = "bEnableControllerVibration"))
	float VibrationDuration = 0.15f;

private:
	/** Handler for the global damage number delegate (reused for feedback). */
	void HandleDamageEvent(AActor* TargetActor, float DamageAmount, bool bIsCrit,
		bool bIsHeal, FGameplayTag DamageType, FVector HitLocation);

	FDelegateHandle GlobalDelegateHandle;

	// --- Hitstop state ---

	/** Actors currently in hitstop, mapped to remaining real time. */
	struct FHitstopEntry
	{
		TWeakObjectPtr<AActor> Actor;
		float RemainingTime;
		float OriginalTimeDilation;
	};
	TArray<FHitstopEntry> ActiveHitstops;

	void ApplyHitstop(AActor* Actor);
	void UpdateHitstops(float DeltaTime);

	// --- Helpers ---
	void PlayCameraShake(bool bIsCrit);
	void SpawnHitVFX(const FVector& Location, const FVector& ImpactNormal);
	void PlayHitSFX(const FVector& Location, FGameplayTag DamageType, bool bIsCrit);
	void PlayControllerVibration(bool bIsCrit);
};

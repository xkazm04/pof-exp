#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "ARPGEnvironmentalHazard.generated.h"

class UAudioComponent;
class UBoxComponent;
class UNavModifierComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class USoundBase;
class UGameplayEffect;

/** Type of environmental hazard — determines visual and gameplay behavior. */
UENUM(BlueprintType)
enum class EHazardType : uint8
{
	FireFloor    UMETA(DisplayName = "Fire Floor"),
	PoisonCloud  UMETA(DisplayName = "Poison Cloud"),
	SpikeTrap    UMETA(DisplayName = "Spike Trap"),
	IceField     UMETA(DisplayName = "Ice Field"),
	Custom       UMETA(DisplayName = "Custom")
};

/**
 * Environmental hazard that applies GE damage to characters in its volume.
 * Supports periodic damage, one-shot traps, and togglable on/off states.
 */
UCLASS()
class POF_API AARPGEnvironmentalHazard : public AActor
{
	GENERATED_BODY()

public:
	AARPGEnvironmentalHazard();

	UFUNCTION(BlueprintCallable, Category = "World|Hazard")
	void ActivateHazard();

	UFUNCTION(BlueprintCallable, Category = "World|Hazard")
	void DeactivateHazard();

	UFUNCTION(BlueprintCallable, Category = "World|Hazard")
	void ToggleHazard();

	UFUNCTION(BlueprintPure, Category = "World|Hazard")
	bool IsActive() const { return bIsActive; }

	UFUNCTION(BlueprintPure, Category = "World|Hazard")
	EHazardType GetHazardType() const { return HazardType; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hazard")
	TObjectPtr<UBoxComponent> DamageVolume;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hazard|VFX")
	TObjectPtr<UNiagaraComponent> HazardVFX;

	/** Hazard type — drives default damage tag and VFX. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	EHazardType HazardType = EHazardType::FireFloor;

	/** Damage per tick while a character stands in the hazard. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Damage", meta = (ClampMin = "0"))
	float DamagePerTick = 15.f;

	/** Time between damage ticks in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Damage", meta = (ClampMin = "0.1"))
	float DamageInterval = 1.0f;

	/** Gameplay effect class applied on each tick. If null, uses GE_Damage with SetByCaller. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** Damage type tag (e.g., Damage.Fire, Damage.Ice). Auto-set from HazardType if empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Damage")
	FGameplayTag DamageTypeTag;

	/** Whether the hazard starts active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	bool bStartActive = true;

	/** If true, damage is one-shot (spike trap style) with a rearm delay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard")
	bool bOneShot = false;

	/** Delay before one-shot hazard rearms. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard", meta = (EditCondition = "bOneShot", ClampMin = "0"))
	float RearmDelay = 3.0f;

	/** If true, the hazard cycles on/off automatically. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Cycle")
	bool bCycleOnOff = false;

	/** Duration the hazard is active during a cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Cycle", meta = (EditCondition = "bCycleOnOff", ClampMin = "0.5"))
	float CycleOnDuration = 3.0f;

	/** Duration the hazard is inactive during a cycle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Cycle", meta = (EditCondition = "bCycleOnOff", ClampMin = "0.5"))
	float CycleOffDuration = 2.0f;

	/** Starting offset into the cycle (for staggering multiple hazards). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Cycle", meta = (EditCondition = "bCycleOnOff", ClampMin = "0"))
	float CyclePhaseOffset = 0.f;

	/** Optional VFX system for the hazard. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|VFX")
	TObjectPtr<UNiagaraSystem> HazardVFXSystem;

	/** Sound played on activation / each tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|SFX")
	TObjectPtr<USoundBase> HazardSound;

	/** Optional secondary effect applied on overlap (e.g., slow, DoT, debuff). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Damage")
	TSubclassOf<UGameplayEffect> DebuffEffectClass;

	/** Whether the hazard damages the player only or all characters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Damage")
	bool bDamagePlayerOnly = false;

	/** If true, adds a NavModifier so AI avoids the hazard area. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|AI")
	bool bAIAvoidance = true;

	/** Grace period before first damage tick on entry (seconds). 0 = instant. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Damage", meta = (ClampMin = "0"))
	float EntryGracePeriod = 0.3f;

	/** If true, multiplies DamagePerTick by the current zone's difficulty multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Damage")
	bool bScaleWithZoneDifficulty = true;

	/** Seconds of warning VFX before a cycling hazard activates (0 = no warning). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|Cycle", meta = (EditCondition = "bCycleOnOff", ClampMin = "0"))
	float WarningDuration = 0.5f;

	/** Optional VFX system played as warning before activation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|VFX")
	TObjectPtr<UNiagaraSystem> WarningVFXSystem;

	/** Optional warning sound played before activation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hazard|SFX")
	TObjectPtr<USoundBase> WarningSound;

private:
	bool bIsActive = false;
	bool bIsRearming = false;

	FTimerHandle DamageTimerHandle;
	FTimerHandle CycleTimerHandle;
	FTimerHandle RearmTimerHandle;

	UPROPERTY()
	TObjectPtr<UAudioComponent> LoopingSoundComponent;

	UPROPERTY()
	TObjectPtr<UNavModifierComponent> HazardNavModifier;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> OverlappingActors;

	/** Actors recently hit by one-shot — immune until they leave and re-enter. */
	UPROPERTY()
	TSet<TWeakObjectPtr<AActor>> OneShotImmune;

	/** Actors that have the debuff applied — cleared on end overlap to avoid re-application spam. */
	UPROPERTY()
	TSet<TWeakObjectPtr<AActor>> DebuffedActors;

	UFUNCTION()
	void OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ApplyDamageToOverlapping();
	void ApplyDamageToActor(AActor* Target);
	void ApplyDebuffToActor(AActor* Target);
	void OnCycleTimer();
	void OnCycleWarningDone();
	void OnRearm();
	FGameplayTag GetDefaultDamageTag() const;

	FTimerHandle WarningTimerHandle;
};

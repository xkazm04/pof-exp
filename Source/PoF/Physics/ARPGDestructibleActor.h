#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGDestructibleActor.generated.h"

class UGeometryCollectionComponent;
class UFieldSystemComponent;
class USoundBase;
class UNiagaraSystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDestructibleBroken, AARPGDestructibleActor*, DestructibleActor, float, DamageReceived);

/**
 * Base class for destructible objects using UE5 Chaos system.
 * Uses a GeometryCollectionComponent for fracture simulation.
 * Tracks accumulated damage and breaks when threshold is exceeded.
 */
UCLASS(BlueprintType, Blueprintable)
class POF_API AARPGDestructibleActor : public AActor
{
	GENERATED_BODY()

public:
	AARPGDestructibleActor();

	/** Apply damage to this destructible. Triggers fracture when threshold is reached. */
	UFUNCTION(BlueprintCallable, Category = "Destructible")
	void ApplyDestructionDamage(float DamageAmount, const FVector& ImpactPoint, const FVector& ImpactNormal, float ImpulseStrength = 500.f);

	/** Force immediate destruction regardless of remaining health. */
	UFUNCTION(BlueprintCallable, Category = "Destructible")
	void ForceDestruct(const FVector& ImpactPoint, float ImpulseStrength = 1000.f);

	/** Whether this destructible has been broken. */
	UFUNCTION(BlueprintPure, Category = "Destructible")
	bool IsBroken() const { return bIsBroken; }

	/** Broadcast when the destructible breaks. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnDestructibleBroken OnBroken;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Destructible")
	TObjectPtr<UGeometryCollectionComponent> GeometryCollection;

	/** Damage required to break this object. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destructible", meta = (ClampMin = "1"))
	float DamageThreshold = 50.f;

	/** Current accumulated damage. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Destructible")
	float AccumulatedDamage = 0.f;

	/** Time in seconds before fractured pieces are cleaned up. 0 = never. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destructible", meta = (ClampMin = "0"))
	float DebrisLifetime = 10.f;

	/** VFX spawned at impact point on break. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destructible|FX")
	TObjectPtr<UNiagaraSystem> BreakVFX;

	/** Sound played on break. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destructible|FX")
	TObjectPtr<USoundBase> BreakSound;

	/** Volume of the break sound. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destructible|FX", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float BreakSoundVolume = 1.0f;

	// --- Loot Drop ---

	/** Actor classes to spawn on break (e.g., item pickups, resources). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destructible|Loot")
	TArray<TSubclassOf<AActor>> LootClasses;

	/** Upward impulse applied to spawned loot items for a pop-out effect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destructible|Loot", meta = (ClampMin = "0"))
	float LootEjectImpulse = 300.f;

	/** Random horizontal spread radius for loot spawn positions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destructible|Loot", meta = (ClampMin = "0"))
	float LootSpawnRadius = 50.f;

	virtual void BeginPlay() override;

private:
	bool bIsBroken = false;

	void TriggerBreak(const FVector& ImpactPoint, const FVector& ImpactNormal, float ImpulseStrength);
	void SpawnLoot();
};

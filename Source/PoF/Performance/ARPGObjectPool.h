#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ARPGObjectPool.generated.h"

/**
 * Generic actor object pool subsystem.
 * Pools frequently spawned actors (projectiles, VFX actors, world items)
 * to avoid repeated spawn/destroy overhead.
 *
 * Usage:
 *   auto* Pool = GetGameInstance()->GetSubsystem<UARPGObjectPool>();
 *   AActor* Actor = Pool->Acquire(AARPGProjectile::StaticClass(), SpawnTransform);
 *   // ... use actor ...
 *   Pool->Release(Actor);
 */
UCLASS()
class POF_API UARPGObjectPool : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Acquire an actor from the pool or spawn a new one if the pool is empty.
	 * The actor is moved to the given transform and re-activated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Performance|Pool", meta = (DeterminesOutputType = "ActorClass"))
	AActor* Acquire(TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform);

	/**
	 * Return an actor to the pool. The actor is deactivated and hidden.
	 * If the pool for this class is at capacity, the actor is destroyed instead.
	 */
	UFUNCTION(BlueprintCallable, Category = "Performance|Pool")
	void Release(AActor* Actor);

	/** Pre-warm the pool by spawning a number of actors ahead of time. */
	UFUNCTION(BlueprintCallable, Category = "Performance|Pool")
	void PreWarm(TSubclassOf<AActor> ActorClass, int32 Count, UWorld* World);

	/** Max actors per class kept in the pool. Excess are destroyed on Release. */
	UFUNCTION(BlueprintCallable, Category = "Performance|Pool")
	void SetMaxPoolSize(TSubclassOf<AActor> ActorClass, int32 MaxSize);

	/** Get current pool stats for debugging. */
	UFUNCTION(BlueprintPure, Category = "Performance|Pool")
	int32 GetPooledCount(TSubclassOf<AActor> ActorClass) const;

	UFUNCTION(BlueprintPure, Category = "Performance|Pool")
	int32 GetActiveCount(TSubclassOf<AActor> ActorClass) const;

private:
	struct FPoolEntry
	{
		TArray<TWeakObjectPtr<AActor>> InactiveActors;
		int32 ActiveCount = 0;
		int32 MaxSize = 32;
	};

	TMap<UClass*, FPoolEntry> Pools;

	void DeactivateActor(AActor* Actor);
	void ActivateActor(AActor* Actor, const FTransform& Transform);
};

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ARPGTickOptimizer.generated.h"

/**
 * Manages tick rate reduction for distant actors.
 *
 * Registered actors have their tick interval adjusted based on
 * distance to the nearest player camera. Close actors tick at
 * full rate; distant actors tick less frequently.
 *
 * Usage:
 *   auto* Opt = GetWorld()->GetSubsystem<UARPGTickOptimizer>();
 *   Opt->RegisterActor(this);             // in BeginPlay
 *   Opt->UnregisterActor(this);           // in EndPlay
 */
UCLASS()
class POF_API UARPGTickOptimizer : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/**
	 * Register an actor for distance-based tick optimization.
	 * The actor's tick interval will be managed by this subsystem.
	 */
	UFUNCTION(BlueprintCallable, Category = "Performance|Tick")
	void RegisterActor(AActor* Actor);

	/** Unregister an actor — restores its original tick interval. */
	UFUNCTION(BlueprintCallable, Category = "Performance|Tick")
	void UnregisterActor(AActor* Actor);

	// =====================================================================
	// Distance Thresholds
	// =====================================================================

	/** Distance within which actors tick at full rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance|Tick")
	float NearDistance = 2000.f;

	/** Distance beyond which actors tick at the slowest rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance|Tick")
	float FarDistance = 8000.f;

	/** Tick interval for near actors (seconds). 0 = every frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance|Tick", meta = (ClampMin = "0"))
	float NearTickInterval = 0.f;

	/** Tick interval for mid-range actors (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance|Tick", meta = (ClampMin = "0"))
	float MidTickInterval = 0.1f;

	/** Tick interval for far actors (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance|Tick", meta = (ClampMin = "0"))
	float FarTickInterval = 0.5f;

	/** How often the optimizer re-evaluates all registered actors (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance|Tick", meta = (ClampMin = "0.1"))
	float EvaluationInterval = 1.0f;

private:
	struct FManagedActor
	{
		TWeakObjectPtr<AActor> Actor;
		float OriginalTickInterval;
	};

	TArray<FManagedActor> ManagedActors;
	float TimeSinceLastEvaluation = 0.f;

	void EvaluateTickRates();
	FVector GetPrimaryCameraLocation() const;
};

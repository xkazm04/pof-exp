#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnVolume.generated.h"

class AARPGEnemyCharacter;
class UBoxComponent;

/** Configuration for a single wave within a spawn volume. */
USTRUCT(BlueprintType)
struct FSpawnWaveConfig
{
	GENERATED_BODY()

	/** Enemy class for this wave. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TSubclassOf<AARPGEnemyCharacter> EnemyClass;

	/** Number of enemies in this wave. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "1"))
	int32 Count = 3;

	/** Delay in seconds between spawning each enemy in this wave. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "0"))
	float SpawnInterval = 0.5f;

	/** Delay in seconds before this wave begins (after the previous wave finishes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "0"))
	float DelayBeforeWave = 0.f;

	/**
	 * Additional difficulty multiplier stacked on top of the volume's base difficulty.
	 * E.g., 1.0 = no extra scaling, 1.5 = 50% harder than base.
	 * Allows later waves to be progressively harder.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave", meta = (ClampMin = "1.0"))
	float WaveDifficultyMultiplier = 1.0f;
};

/** Broadcast when a wave starts: (WaveIndex, TotalWaves) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWaveStarted, int32, WaveIndex, int32, TotalWaves);

/** Broadcast when all waves are complete. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllWavesComplete);

/**
 * A trigger volume that spawns waves of enemies when the player enters.
 *
 * Configure Waves array with enemy classes, counts, and timing.
 * Difficulty scaling is computed automatically from the player's level
 * relative to BaseDifficultyLevel.
 */
UCLASS()
class POF_API ASpawnVolume : public AActor
{
	GENERATED_BODY()

public:
	ASpawnVolume();

	/** Manually start the wave sequence (bypasses trigger). */
	UFUNCTION(BlueprintCallable, Category = "Spawning")
	void StartWaves();

	/** Whether waves are currently active. */
	UFUNCTION(BlueprintPure, Category = "Spawning")
	bool IsSpawning() const { return bIsSpawning; }

	/** Get the current wave index (0-based). -1 if not spawning. */
	UFUNCTION(BlueprintPure, Category = "Spawning")
	int32 GetCurrentWaveIndex() const { return CurrentWaveIndex; }

	/** Count of living enemies spawned by this volume. */
	UFUNCTION(BlueprintPure, Category = "Spawning")
	int32 GetLivingEnemyCount() const;

	UPROPERTY(BlueprintAssignable, Category = "Events|Spawning")
	FOnWaveStarted OnWaveStarted;

	UPROPERTY(BlueprintAssignable, Category = "Events|Spawning")
	FOnAllWavesComplete OnAllWavesComplete;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Trigger volume — player overlap starts wave spawning. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawning")
	TObjectPtr<UBoxComponent> TriggerVolume;

	/** Wave configurations in order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning|Waves")
	TArray<FSpawnWaveConfig> Waves;

	/** If true, waves start when the player enters the trigger volume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	bool bTriggerOnOverlap = true;

	/**
	 * If true, the next wave waits until all enemies from the current wave are dead.
	 * If false, waves advance immediately after spawning (overlapping waves).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning|Waves")
	bool bWaitForKillsBeforeNextWave = true;

	/** If true, waves can be triggered again after all enemies are dead and a reset delay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	bool bCanRetrigger = false;

	/** Delay before allowing re-trigger after all waves complete. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (EditCondition = "bCanRetrigger", ClampMin = "0"))
	float RetriggerDelay = 60.f;

	/**
	 * Base difficulty level of enemies in this volume.
	 * Difficulty multiplier = 1 + (PlayerLevel - BaseDifficultyLevel) * DifficultyScalePerLevel.
	 * Clamped to a minimum of 1.0 (enemies never get weaker than base).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning|Difficulty", meta = (ClampMin = "1"))
	int32 BaseDifficultyLevel = 1;

	/** How much each player level above BaseDifficultyLevel adds to the multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning|Difficulty", meta = (ClampMin = "0"))
	float DifficultyScalePerLevel = 0.1f;

	/** Maximum difficulty multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning|Difficulty", meta = (ClampMin = "1.0"))
	float MaxDifficultyMultiplier = 3.0f;

	/** Maximum concurrent living enemies this volume will maintain. 0 = unlimited. Spawns are deferred until count drops. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = "0"))
	int32 MaxConcurrentEnemies = 0;

private:
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void BeginNextWave();
	void SpawnNextInWave();
	AARPGEnemyCharacter* SpawnEnemyInVolume(TSubclassOf<AARPGEnemyCharacter> EnemyClass);
	FVector GetRandomSpawnLocationInVolume() const;
	float ComputeDifficultyMultiplier() const;
	void ApplyDifficultyScaling(AARPGEnemyCharacter* Enemy, float Multiplier);

	UFUNCTION()
	void OnSpawnedEnemyDied(AARPGEnemyCharacter* DeadEnemy);

	void CheckWaveCompletion();

	bool bIsSpawning = false;
	bool bHasTriggered = false;
	bool bWaitingForKills = false;
	int32 CurrentWaveIndex = -1;
	int32 CurrentWaveSpawnedCount = 0;

	FTimerHandle WaveDelayTimerHandle;
	FTimerHandle SpawnIntervalTimerHandle;
	FTimerHandle RetriggerTimerHandle;

	UPROPERTY()
	TArray<TWeakObjectPtr<AARPGEnemyCharacter>> SpawnedEnemies;

	/** Enemies spawned in the current wave only (for kill-based advancement). */
	UPROPERTY()
	TArray<TWeakObjectPtr<AARPGEnemyCharacter>> CurrentWaveEnemies;
};

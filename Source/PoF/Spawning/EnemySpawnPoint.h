#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawnPoint.generated.h"

class AARPGEnemyCharacter;
class UBillboardComponent;
class UARPGZoneDefinition;

/**
 * A single spawn point that spawns one enemy and optionally respawns it
 * after a configurable delay when the enemy dies.
 *
 * Place in the world, set EnemyClass and RespawnDelay.
 * The spawn point handles death binding and respawn timers automatically.
 */
UCLASS()
class POF_API AEnemySpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AEnemySpawnPoint();

	/** Manually trigger a spawn (ignores respawn timer). */
	UFUNCTION(BlueprintCallable, Category = "Spawning")
	AARPGEnemyCharacter* SpawnEnemy();

	/** Spawn with an explicit difficulty multiplier applied via GE. */
	UFUNCTION(BlueprintCallable, Category = "Spawning")
	AARPGEnemyCharacter* SpawnEnemyWithDifficulty(float DifficultyMultiplier);

	/** Whether this spawn point currently has a living enemy. */
	UFUNCTION(BlueprintPure, Category = "Spawning")
	bool HasLivingEnemy() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Enemy class to spawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TSubclassOf<AARPGEnemyCharacter> EnemyClass;

	/** If true, spawns an enemy automatically on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	bool bSpawnOnBeginPlay = true;

	/** Respawn delay in seconds after the enemy dies. 0 or negative = no respawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = "-1"))
	float RespawnDelay = 30.f;

	/** Override the spawned enemy's CharacterLevel. 0 = use the class default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning|Difficulty", meta = (ClampMin = "0"))
	int32 EnemyLevelOverride = 0;

	/** Random radius around the spawn point for spawn location jitter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning", meta = (ClampMin = "0"))
	float SpawnRadius = 0.f;

	/** Optional zone this spawn point belongs to. When set, enemy level defaults to zone's BaseEnemyLevel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning|Zone")
	TObjectPtr<UARPGZoneDefinition> ZoneDefinition;

private:
	UPROPERTY()
	TWeakObjectPtr<AARPGEnemyCharacter> CurrentEnemy;

	FTimerHandle RespawnTimerHandle;

	UFUNCTION()
	void OnEnemyDied(AARPGEnemyCharacter* DeadEnemy);

	void ScheduleRespawn();

	FVector GetSpawnLocation() const;

	void ApplyDifficultyScaling(AARPGEnemyCharacter* Enemy, float Multiplier);

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Spawning")
	TObjectPtr<UBillboardComponent> EditorSprite;
#endif
};

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGBossEncounter.generated.h"

class AARPGBossCharacter;
class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossEncounterStarted, AARPGBossCharacter*, Boss);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossEncounterCompleted, AARPGBossCharacter*, Boss);

/**
 * Boss arena encounter manager.
 * When the player enters the arena trigger, the boss is activated,
 * fog walls close, and the fight begins. On boss death, rewards are
 * granted and the arena reopens.
 */
UCLASS()
class POF_API AARPGBossEncounter : public AActor
{
	GENERATED_BODY()

public:
	AARPGBossEncounter();

	UFUNCTION(BlueprintCallable, Category = "Boss|Encounter")
	void StartEncounter();

	UFUNCTION(BlueprintPure, Category = "Boss|Encounter")
	bool IsEncounterActive() const { return bEncounterActive; }

	UFUNCTION(BlueprintPure, Category = "Boss|Encounter")
	AARPGBossCharacter* GetBoss() const;

	UPROPERTY(BlueprintAssignable, Category = "Events|Boss")
	FOnBossEncounterStarted OnEncounterStarted;

	UPROPERTY(BlueprintAssignable, Category = "Events|Boss")
	FOnBossEncounterCompleted OnEncounterCompleted;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Trigger volume for the boss arena. Player overlap starts the fight. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Encounter")
	TObjectPtr<UBoxComponent> ArenaTrigger;

	/** Boss class to spawn. If null, expects BossSpawnPoint to have a pre-placed boss. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Encounter")
	TSubclassOf<AARPGBossCharacter> BossClass;

	/** Where to spawn the boss. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Encounter", meta = (MakeEditWidget = true))
	FVector BossSpawnOffset = FVector(0.f, 0.f, 0.f);

	/** Pre-placed boss actor (alternative to BossClass spawning). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Encounter")
	TObjectPtr<AARPGBossCharacter> PrePlacedBoss;

	/** If true, trigger on player overlap. If false, must call StartEncounter manually. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Encounter")
	bool bTriggerOnOverlap = true;

	/** If true, can be fought again after reset delay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Encounter")
	bool bCanRetrigger = false;

	/** Delay before the encounter can be retriggered after boss death. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Encounter", meta = (EditCondition = "bCanRetrigger", ClampMin = "0"))
	float RetriggerDelay = 300.f;

	/** Actors to show/hide when the encounter starts (fog walls, barriers). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Encounter|Arena")
	TArray<TObjectPtr<AActor>> FogWallActors;

	/** Level override for the spawned boss. 0 = use class default. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Encounter", meta = (ClampMin = "0"))
	int32 BossLevelOverride = 0;

private:
	bool bEncounterActive = false;
	bool bEncounterCompleted = false;

	UPROPERTY()
	TObjectPtr<AARPGBossCharacter> SpawnedBoss;

	FTimerHandle RetriggerTimerHandle;

	UFUNCTION()
	void OnArenaOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void SpawnBoss();
	void ActivateFogWalls(bool bActivate);

	UFUNCTION()
	void OnBossDied(AARPGEnemyCharacter* DeadEnemy);

	void CompleteEncounter();

	UFUNCTION()
	void OnPlayerDied();
};

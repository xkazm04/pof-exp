#pragma once

#include "CoreMinimal.h"
#include "Character/ARPGCharacterBase.h"
#include "ARPGCombatTestDummy.generated.h"

class UWidgetComponent;
class UEnemyHealthBarWidget;

/**
 * Static test dummy for validating the full attack-damage-death combat loop.
 *
 * Features:
 *   - Stands still (no AI, no movement)
 *   - Has GAS with health attributes, takes damage through the normal pipeline
 *   - Displays a floating health bar
 *   - Plays hit react and death montages via GA_HitReact / GA_Death
 *   - Auto-respawns after death (configurable delay)
 *   - Optional invulnerability toggle for infinite-health testing
 *
 * Place in a level to test melee combos, damage numbers, hitstop, camera shake,
 * VFX, and the full death -> ragdoll -> respawn flow.
 */
UCLASS()
class POF_API AARPGCombatTestDummy : public AARPGCharacterBase
{
	GENERATED_BODY()

public:
	AARPGCombatTestDummy();

	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

	virtual bool IsDead() const override { return !bIsAlive; }

	// =====================================================================
	// Configuration
	// =====================================================================

	/** If true, the dummy respawns in place after dying. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestDummy")
	bool bAutoRespawn = true;

	/** Delay before respawning after death. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestDummy", meta = (ClampMin = "0.5", ClampMax = "30.0", EditCondition = "bAutoRespawn"))
	float RespawnDelay = 3.0f;

	/** If true, the dummy cannot die (health resets to max after each hit). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestDummy")
	bool bInvulnerable = false;

	/** Display name shown on the floating health bar. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestDummy|UI")
	FText DisplayName = FText::FromString(TEXT("Test Dummy"));

	/** Widget class for the floating health bar. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TestDummy|UI")
	TSubclassOf<UEnemyHealthBarWidget> HealthBarWidgetClass;

	/**
	 * Called by GA_Death when this dummy dies.
	 * Schedules respawn if bAutoRespawn is true.
	 */
	void OnDeathFromAbility(AActor* KillingActor = nullptr);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> HealthBarWidgetComp;

private:
	bool bIsAlive = true;

	FVector SpawnLocation;
	FRotator SpawnRotation;

	FTimerHandle RespawnTimerHandle;

	void GrantAbilities();
	void InitializeHealthBarWidget();
	void ExecuteRespawn();
};

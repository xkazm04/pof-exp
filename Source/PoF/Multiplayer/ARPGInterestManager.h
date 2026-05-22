#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ARPGInterestManager.generated.h"

/**
 * Relevancy tier that determines how aggressively an actor is replicated.
 * Lower tiers get higher update frequency and priority.
 */
UENUM(BlueprintType)
enum class EARPGRelevancyTier : uint8
{
	/** Always replicated regardless of distance (GameState, GameMode). */
	AlwaysRelevant,

	/** Critical actors that need frequent updates (players, bosses). */
	Critical,

	/** Standard actors with distance-based culling (enemies, NPCs). */
	Standard,

	/** Low-priority actors with aggressive culling (distant props, items). */
	Background,

	/** Dormant actors that only replicate on significant state changes. */
	Dormant
};

/**
 * Configuration for a relevancy tier.
 */
USTRUCT(BlueprintType)
struct FARPGRelevancyTierConfig
{
	GENERATED_BODY()

	/** Maximum distance (cm) at which this actor is relevant. 0 = infinite. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relevancy")
	float MaxRelevancyDistance = 0.f;

	/** Net update frequency (times per second) for this tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relevancy", meta = (ClampMin = "0.1"))
	float UpdateFrequency = 10.f;

	/** Net priority multiplier. Higher = more bandwidth allocated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relevancy", meta = (ClampMin = "0.1"))
	float PriorityMultiplier = 1.f;
};

/**
 * Interest management component for network relevancy.
 *
 * Attach to any replicated actor to control its replication behavior.
 * Integrates with UE 5.7 Iris replication system via the owning actor's
 * FillReplicationParams override.
 *
 * Features:
 *  - Tier-based relevancy with configurable distance/frequency/priority
 *  - Dynamic tier switching based on gameplay state (e.g., entering combat)
 *  - Owner distance tracking for adaptive net update frequency
 *  - Dormancy support for idle actors
 */
UCLASS(ClassGroup = (Multiplayer), meta = (BlueprintSpawnableComponent))
class POF_API UARPGInterestManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGInterestManager();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// =====================================================================
	// Tier Management
	// =====================================================================

	/** Get the current relevancy tier. */
	UFUNCTION(BlueprintPure, Category = "Relevancy")
	EARPGRelevancyTier GetRelevancyTier() const { return CurrentTier; }

	/** Set the relevancy tier. Updates the owning actor's net update frequency. */
	UFUNCTION(BlueprintCallable, Category = "Relevancy")
	void SetRelevancyTier(EARPGRelevancyTier NewTier);

	/** Get the config for a specific tier. */
	UFUNCTION(BlueprintPure, Category = "Relevancy")
	FARPGRelevancyTierConfig GetTierConfig(EARPGRelevancyTier Tier) const;

	// =====================================================================
	// Dynamic State
	// =====================================================================

	/** Enter combat mode — bumps relevancy to Critical temporarily. */
	UFUNCTION(BlueprintCallable, Category = "Relevancy|State")
	void EnterCombatRelevancy();

	/** Exit combat mode — returns to base tier. */
	UFUNCTION(BlueprintCallable, Category = "Relevancy|State")
	void ExitCombatRelevancy();

	/** Whether the actor is in combat-elevated relevancy. */
	UFUNCTION(BlueprintPure, Category = "Relevancy|State")
	bool IsInCombatRelevancy() const { return bInCombatRelevancy; }

	/** Mark the actor as dormant (stops replication until woken). */
	UFUNCTION(BlueprintCallable, Category = "Relevancy|State")
	void EnterDormancy();

	/** Wake from dormancy and resume replication. */
	UFUNCTION(BlueprintCallable, Category = "Relevancy|State")
	void WakeFromDormancy();

	/** Whether the actor is currently dormant. */
	UFUNCTION(BlueprintPure, Category = "Relevancy|State")
	bool IsDormant() const { return bIsDormant; }

	// =====================================================================
	// Distance Queries
	// =====================================================================

	/** Distance to the nearest player (updated each tick on server). */
	UFUNCTION(BlueprintPure, Category = "Relevancy|Distance")
	float GetDistanceToNearestPlayer() const { return DistanceToNearestPlayer; }

	/** Whether this actor is within its relevancy distance of any player. */
	UFUNCTION(BlueprintPure, Category = "Relevancy|Distance")
	bool IsRelevantToAnyPlayer() const { return bIsRelevant; }

protected:
	/** Base relevancy tier for this actor (set in defaults, restored when combat ends). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relevancy")
	EARPGRelevancyTier BaseTier = EARPGRelevancyTier::Standard;

	/** Per-tier configuration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relevancy")
	TMap<EARPGRelevancyTier, FARPGRelevancyTierConfig> TierConfigs;

	/** How often (seconds) to recalculate distance to players. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relevancy|Tuning", meta = (ClampMin = "0.1"))
	float DistanceCheckInterval = 0.5f;

	/** When true, automatically adjusts tier based on distance bands. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relevancy|Tuning")
	bool bAutoTierByDistance = true;

	/** Distance band boundaries for auto-tier: [0, CriticalDist) = Critical, [CriticalDist, StandardDist) = Standard, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relevancy|Tuning", meta = (EditCondition = "bAutoTierByDistance"))
	float CriticalDistanceBand = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relevancy|Tuning", meta = (EditCondition = "bAutoTierByDistance"))
	float StandardDistanceBand = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Relevancy|Tuning", meta = (EditCondition = "bAutoTierByDistance"))
	float BackgroundDistanceBand = 10000.f;

private:
	EARPGRelevancyTier CurrentTier = EARPGRelevancyTier::Standard;
	bool bInCombatRelevancy = false;
	bool bIsDormant = false;
	bool bIsRelevant = true;
	float DistanceToNearestPlayer = 0.f;
	float DistanceCheckTimer = 0.f;

	void InitDefaultTierConfigs();
	void UpdateDistanceToPlayers();
	void UpdateAutoTier();
	void ApplyTierSettings();
};

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "World/ARPGZoneDefinition.h"
#include "ARPGZoneManager.generated.h"

/** Broadcast when the player enters a new zone. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnZoneChanged, UARPGZoneDefinition*, NewZone, UARPGZoneDefinition*, PreviousZone);

/** Broadcast when a zone's streaming level finishes loading. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnZoneLoaded, UARPGZoneDefinition*, Zone);

/**
 * Manages the world zone graph, current zone tracking, zone transitions,
 * and level streaming coordination.
 */
UCLASS()
class POF_API UARPGZoneManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// =====================================================================
	// Zone Registration
	// =====================================================================

	/** Register a zone definition. Called by ZoneVolumes on BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "World|Zones")
	void RegisterZone(UARPGZoneDefinition* ZoneDef);

	/** Get all registered zones. */
	UFUNCTION(BlueprintPure, Category = "World|Zones")
	TArray<UARPGZoneDefinition*> GetAllZones() const;

	/** Find a zone by its ID. */
	UFUNCTION(BlueprintPure, Category = "World|Zones")
	UARPGZoneDefinition* GetZoneByID(FName ZoneID) const;

	// =====================================================================
	// Current Zone
	// =====================================================================

	/** Get the zone the player is currently in. */
	UFUNCTION(BlueprintPure, Category = "World|Zones")
	UARPGZoneDefinition* GetCurrentZone() const { return CurrentZone; }

	/** Set the player's current zone. Fires OnZoneChanged. */
	UFUNCTION(BlueprintCallable, Category = "World|Zones")
	void SetCurrentZone(UARPGZoneDefinition* NewZone);

	/** Whether the player is in a safe zone. */
	UFUNCTION(BlueprintPure, Category = "World|Zones")
	bool IsInSafeZone() const;

	// =====================================================================
	// Zone Queries
	// =====================================================================

	/** Get the difficulty multiplier for the current zone. */
	UFUNCTION(BlueprintPure, Category = "World|Zones")
	float GetCurrentZoneDifficulty() const;

	/** Get connections from the current zone. */
	UFUNCTION(BlueprintPure, Category = "World|Zones")
	TArray<FZoneConnection> GetCurrentZoneConnections() const;

	/** Check if a zone connection is unlocked for the player. */
	UFUNCTION(BlueprintPure, Category = "World|Zones")
	bool IsConnectionUnlocked(const FZoneConnection& Connection) const;

	/** Get zones the player can fast-travel to. */
	UFUNCTION(BlueprintPure, Category = "World|Zones")
	TArray<UARPGZoneDefinition*> GetFastTravelDestinations() const;

	// =====================================================================
	// Level Streaming
	// =====================================================================

	/** Request a zone transition — handles streaming load/unload with screen fade. */
	UFUNCTION(BlueprintCallable, Category = "World|Zones")
	void RequestZoneTransition(UARPGZoneDefinition* TargetZone);

	/** Request a zone transition and teleport to a specific spawn point. */
	UFUNCTION(BlueprintCallable, Category = "World|Zones")
	void RequestZoneTransitionToSpawn(UARPGZoneDefinition* TargetZone, FName SpawnPointTag);

	// =====================================================================
	// Delegates
	// =====================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events|World")
	FOnZoneChanged OnZoneChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events|World")
	FOnZoneLoaded OnZoneLoaded;

private:
	UPROPERTY()
	TMap<FName, TObjectPtr<UARPGZoneDefinition>> RegisteredZones;

	UPROPERTY()
	TObjectPtr<UARPGZoneDefinition> CurrentZone;

	/** Track which zones have been visited (for fast travel unlock). */
	UPROPERTY()
	TSet<FName> VisitedZoneIDs;

	void HandleStreamingLevelLoaded(UARPGZoneDefinition* Zone);
	void TeleportPlayerToSpawn(FName SpawnPointTag);
	void FadeScreenAndTransition(UARPGZoneDefinition* TargetZone, FName SpawnPointTag);
	void OnFadeOutComplete(UARPGZoneDefinition* TargetZone, FName SpawnPointTag);
	void PollStreamingLevelLoaded();

	/** Pending spawn point for the current transition. */
	FName PendingSpawnTag;

	/** Whether a transition is in progress (prevents double-triggers). */
	bool bTransitionInProgress = false;

	/** Zone being streamed in (for polling). */
	UPROPERTY()
	TObjectPtr<UARPGZoneDefinition> PendingStreamingZone;

	FTimerHandle StreamingPollHandle;
	int32 StreamingPollCount = 0;
};

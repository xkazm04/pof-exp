#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Framework/ARPGSaveGame.h"
#include "ARPGGameInstance.generated.h"

class UARPGSaveSubsystem;

// --- Level transition delegates ---

/** Broadcast before a map change begins: (CurrentMapName, NextMapName) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPreMapChange, const FString&, CurrentMap, const FString&, NextMap);

/** Broadcast after a map change completes: (NewMapName) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPostMapChange, const FString&, NewMap);

UCLASS()
class POF_API UARPGGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	// =====================================================================
	// Play Time Tracking
	// =====================================================================

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetTotalPlayTime() const { return TotalPlayTime; }

	// =====================================================================
	// Progression (in-memory working copy)
	// =====================================================================

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Player")
	int32 PlayerLevel = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Player")
	float Experience = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Player")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Player")
	float MaxMana = 50.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "World")
	FString CurrentZone;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	float TotalPlayTime = 0.f;

	// =====================================================================
	// Map Transitions
	// =====================================================================

	/** Request a level transition. Broadcasts delegates and performs the travel. */
	UFUNCTION(BlueprintCallable, Category = "World")
	void TravelToMap(const FString& MapName);

	// =====================================================================
	// Save System Access
	// =====================================================================

	/** Convenience getter for the save subsystem. */
	UFUNCTION(BlueprintPure, Category = "Save")
	UARPGSaveSubsystem* GetSaveSubsystem() const;

	/** Quick save through the subsystem. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool QuickSave();

	/** Quick load through the subsystem. */
	UFUNCTION(BlueprintCallable, Category = "Save")
	bool QuickLoad();

	// =====================================================================
	// Subsystem data sync (called by UARPGSaveSubsystem)
	// =====================================================================

	/** Push current GI state into the subsystem's cache before saving. */
	void SyncProgressionToSubsystem();

	/** Pull loaded data from subsystem into GI state. */
	void SyncProgressionFromSubsystem(const FARPGPlayerProgression& LoadedProgression);

	// =====================================================================
	// Delegates
	// =====================================================================

	UPROPERTY(BlueprintAssignable, Category = "Events|World")
	FOnPreMapChange OnPreMapChange;

	UPROPERTY(BlueprintAssignable, Category = "Events|World")
	FOnPostMapChange OnPostMapChange;

private:
	/** Timer handle for play time accumulation. */
	FTimerHandle PlayTimeTimerHandle;

	/** Called every second to increment play time. */
	void TickPlayTime();

	/** Bound to FCoreUObjectDelegates::PostLoadMapWithWorld to detect map loads. */
	void HandlePostLoadMap(UWorld* LoadedWorld);
};

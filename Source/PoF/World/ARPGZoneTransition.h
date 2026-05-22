#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGZoneTransition.generated.h"

class UARPGZoneDefinition;
class UBoxComponent;
class UStaticMeshComponent;
class UArrowComponent;

/** Broadcast when the player interacts with this transition. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnZoneTransitionTriggered, UARPGZoneDefinition*, TargetZone, AARPGZoneTransition*, Transition);

/**
 * A portal/doorway between two zones.
 * Placed at zone boundaries. When the player overlaps or interacts,
 * it triggers a zone transition through the ZoneManager.
 *
 * Can optionally gate on level or quest completion.
 */
UCLASS()
class POF_API AARPGZoneTransition : public AActor
{
	GENERATED_BODY()

public:
	AARPGZoneTransition();

	/** Check if the player meets requirements to use this transition. */
	UFUNCTION(BlueprintPure, Category = "World|Zones")
	bool CanPlayerTransition() const;

	/** Get the reason the player can't transition (empty if allowed). */
	UFUNCTION(BlueprintPure, Category = "World|Zones")
	FText GetBlockedReason() const;

	UPROPERTY(BlueprintAssignable, Category = "Events|World")
	FOnZoneTransitionTriggered OnTransitionTriggered;

	/** Try to use this transition (for non-overlap interact mode). */
	UFUNCTION(BlueprintCallable, Category = "World|Zones")
	void TryInteractTransition();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** The zone this portal leads to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	TObjectPtr<UARPGZoneDefinition> TargetZone;

	/** Spawn point in the target zone (tag name to find). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	FName TargetSpawnPointTag;

	/** If true, trigger on overlap. If false, requires interact input. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition")
	bool bTriggerOnOverlap = true;

	/** Minimum player level to use this transition. 0 = no gate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition|Requirements", meta = (ClampMin = "0"))
	int32 RequiredLevel = 0;

	/** Quest that must be completed to use this transition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition|Requirements")
	FName RequiredQuestID;

	/** Trigger volume. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transition")
	TObjectPtr<UBoxComponent> TriggerVolume;

	/** Optional visual mesh (archway, door, portal effect). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transition")
	TObjectPtr<UStaticMeshComponent> PortalMesh;

	/** Direction arrow for editor visualization. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Transition")
	TObjectPtr<UArrowComponent> DirectionArrow;

	/** Minimum delay between transitions from this portal (prevents rapid re-trigger). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transition", meta = (ClampMin = "0"))
	float TransitionCooldown = 2.0f;

private:
	bool bOnCooldown = false;
	FTimerHandle CooldownTimerHandle;

	UFUNCTION()
	void OnPlayerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void ExecuteTransition();
};

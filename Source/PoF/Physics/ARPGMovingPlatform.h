#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGMovingPlatform.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWaypointReached, AARPGMovingPlatform*, Platform, int32, WaypointIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlatformMovementChanged, bool, bMoving);

/**
 * A platform that moves between waypoints using InterpTo.
 * Characters standing on it are carried along via the physics base.
 * Supports pause-at-waypoint, looping, and ping-pong modes.
 */
UCLASS(BlueprintType, Blueprintable)
class POF_API AARPGMovingPlatform : public AActor
{
	GENERATED_BODY()

public:
	AARPGMovingPlatform();

	virtual void Tick(float DeltaTime) override;

	/** Start or resume platform movement. */
	UFUNCTION(BlueprintCallable, Category = "Platform")
	void StartMoving();

	/** Stop/pause platform movement. */
	UFUNCTION(BlueprintCallable, Category = "Platform")
	void StopMoving();

	/** Whether the platform is currently moving. */
	UFUNCTION(BlueprintPure, Category = "Platform")
	bool IsMoving() const { return bIsMoving; }

	/** Get the current waypoint index. */
	UFUNCTION(BlueprintPure, Category = "Platform")
	int32 GetCurrentWaypointIndex() const { return CurrentWaypointIndex; }

	/** Broadcast when the platform reaches a waypoint. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnWaypointReached OnWaypointReached;

	/** Broadcast when movement starts or stops. */
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlatformMovementChanged OnMovementChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Platform")
	TObjectPtr<UStaticMeshComponent> PlatformMesh;

	/** Waypoints in world space. The platform moves between these in order. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform", meta = (MakeEditWidget = true))
	TArray<FVector> Waypoints;

	/** Movement speed (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform", meta = (ClampMin = "10"))
	float MoveSpeed = 200.f;

	/** Seconds to pause at each waypoint before moving to the next. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform", meta = (ClampMin = "0"))
	float WaypointPauseTime = 1.0f;

	/** If true, the platform reverses direction at the end instead of looping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	bool bPingPong = true;

	/** If true, platform starts moving on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform")
	bool bAutoStart = true;

	/** Tolerance distance to consider a waypoint "reached". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Platform", meta = (ClampMin = "1"))
	float ArrivalTolerance = 5.f;

	virtual void BeginPlay() override;

private:
	bool bIsMoving = false;
	int32 CurrentWaypointIndex = 0;
	int32 WaypointDirection = 1; // 1 = forward, -1 = reverse (ping-pong)
	float PauseRemaining = 0.f;

	void AdvanceWaypoint();
};

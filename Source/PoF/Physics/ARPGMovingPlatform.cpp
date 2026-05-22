#include "Physics/ARPGMovingPlatform.h"
#include "Components/StaticMeshComponent.h"

AARPGMovingPlatform::AARPGMovingPlatform()
{
	PrimaryActorTick.bCanEverTick = true;

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));
	SetRootComponent(PlatformMesh);
	PlatformMesh->SetMobility(EComponentMobility::Movable);
	PlatformMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void AARPGMovingPlatform::BeginPlay()
{
	Super::BeginPlay();

	// Convert relative waypoints to world space (MakeEditWidget gives local offsets)
	const FVector Origin = GetActorLocation();
	for (FVector& WP : Waypoints)
	{
		WP = Origin + WP;
	}

	// Insert the starting position as the first waypoint if not already there
	if (Waypoints.Num() > 0 && !Origin.Equals(Waypoints[0], ArrivalTolerance))
	{
		Waypoints.Insert(Origin, 0);
	}

	if (bAutoStart && Waypoints.Num() >= 2)
	{
		StartMoving();
	}
}

void AARPGMovingPlatform::StartMoving()
{
	if (Waypoints.Num() < 2) return;
	if (!bIsMoving)
	{
		bIsMoving = true;
		OnMovementChanged.Broadcast(true);
	}
}

void AARPGMovingPlatform::StopMoving()
{
	if (bIsMoving)
	{
		bIsMoving = false;
		OnMovementChanged.Broadcast(false);
	}
}

void AARPGMovingPlatform::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsMoving || Waypoints.Num() < 2) return;

	// Handle waypoint pause
	if (PauseRemaining > 0.f)
	{
		PauseRemaining -= DeltaTime;
		return;
	}

	// Move toward current target waypoint
	const FVector CurrentLocation = GetActorLocation();
	const FVector& TargetLocation = Waypoints[CurrentWaypointIndex];

	const FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, TargetLocation, DeltaTime, MoveSpeed);
	SetActorLocation(NewLocation, true); // Sweep to push overlapping actors

	// Check if we've reached the waypoint
	if (FVector::Dist(NewLocation, TargetLocation) <= ArrivalTolerance)
	{
		SetActorLocation(TargetLocation, true);
		PauseRemaining = WaypointPauseTime;

		OnWaypointReached.Broadcast(this, CurrentWaypointIndex);
		AdvanceWaypoint();
	}
}

void AARPGMovingPlatform::AdvanceWaypoint()
{
	const int32 NextIndex = CurrentWaypointIndex + WaypointDirection;

	if (NextIndex < 0 || NextIndex >= Waypoints.Num())
	{
		if (bPingPong)
		{
			WaypointDirection = -WaypointDirection;
			CurrentWaypointIndex += WaypointDirection;
		}
		else
		{
			// Loop back to start
			CurrentWaypointIndex = 0;
		}
	}
	else
	{
		CurrentWaypointIndex = NextIndex;
	}
}

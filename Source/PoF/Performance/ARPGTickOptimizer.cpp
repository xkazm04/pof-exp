#include "Performance/ARPGTickOptimizer.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogARPGTick, Log, All);

void UARPGTickOptimizer::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogARPGTick, Display, TEXT("TickOptimizer subsystem initialized"));
}

void UARPGTickOptimizer::Deinitialize()
{
	// Restore original tick intervals
	for (FManagedActor& Entry : ManagedActors)
	{
		if (AActor* Actor = Entry.Actor.Get())
		{
			Actor->PrimaryActorTick.TickInterval = Entry.OriginalTickInterval;
		}
	}
	ManagedActors.Empty();

	Super::Deinitialize();
}

TStatId UARPGTickOptimizer::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UARPGTickOptimizer, STATGROUP_Tickables);
}

void UARPGTickOptimizer::RegisterActor(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	// Don't double-register
	for (const FManagedActor& Entry : ManagedActors)
	{
		if (Entry.Actor.Get() == Actor)
		{
			return;
		}
	}

	FManagedActor NewEntry;
	NewEntry.Actor = Actor;
	NewEntry.OriginalTickInterval = Actor->PrimaryActorTick.TickInterval;
	ManagedActors.Add(NewEntry);
}

void UARPGTickOptimizer::UnregisterActor(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	for (int32 i = ManagedActors.Num() - 1; i >= 0; --i)
	{
		if (ManagedActors[i].Actor.Get() == Actor)
		{
			Actor->PrimaryActorTick.TickInterval = ManagedActors[i].OriginalTickInterval;
			ManagedActors.RemoveAtSwap(i);
			return;
		}
	}
}

void UARPGTickOptimizer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastEvaluation += DeltaTime;
	if (TimeSinceLastEvaluation < EvaluationInterval)
	{
		return;
	}
	TimeSinceLastEvaluation = 0.f;

	EvaluateTickRates();
}

void UARPGTickOptimizer::EvaluateTickRates()
{
	const FVector CameraLocation = GetPrimaryCameraLocation();
	const float NearDistSq = NearDistance * NearDistance;
	const float FarDistSq = FarDistance * FarDistance;

	for (int32 i = ManagedActors.Num() - 1; i >= 0; --i)
	{
		AActor* Actor = ManagedActors[i].Actor.Get();
		if (!Actor)
		{
			ManagedActors.RemoveAtSwap(i);
			continue;
		}

		const float DistSq = FVector::DistSquared(Actor->GetActorLocation(), CameraLocation);

		float NewInterval;
		if (DistSq <= NearDistSq)
		{
			NewInterval = NearTickInterval;
		}
		else if (DistSq >= FarDistSq)
		{
			NewInterval = FarTickInterval;
		}
		else
		{
			// Lerp between mid and far based on distance
			const float Alpha = (FMath::Sqrt(DistSq) - NearDistance) / FMath::Max(FarDistance - NearDistance, 1.f);
			NewInterval = FMath::Lerp(MidTickInterval, FarTickInterval, Alpha);
		}

		Actor->PrimaryActorTick.TickInterval = NewInterval;
	}
}

FVector UARPGTickOptimizer::GetPrimaryCameraLocation() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return FVector::ZeroVector;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return FVector::ZeroVector;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	return ViewLocation;
}

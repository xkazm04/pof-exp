#include "Physics/ARPGTraceUtilities.h"
#include "Physics/ARPGPhysicalMaterial.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"

static FCollisionQueryParams MakeQueryParams(const TArray<AActor*>& ActorsToIgnore)
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ARPGTrace), false);
	Params.bReturnPhysicalMaterial = true;
	for (AActor* Actor : ActorsToIgnore)
	{
		if (Actor)
		{
			Params.AddIgnoredActor(Actor);
		}
	}
	return Params;
}

bool UARPGTraceUtilities::LineTraceByChannel(
	const UObject* WorldContextObject,
	FHitResult& OutHit,
	const FVector& Start,
	const FVector& End,
	ECollisionChannel Channel,
	const TArray<AActor*>& ActorsToIgnore,
	bool bDrawDebug,
	float DebugDuration)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return false;

	FCollisionQueryParams Params = MakeQueryParams(ActorsToIgnore);
	bool bHit = World->LineTraceSingleByChannel(OutHit, Start, End, Channel, Params);

#if ENABLE_DRAW_DEBUG
	if (bDrawDebug)
	{
		DrawDebugLine(World, Start, bHit ? OutHit.ImpactPoint : End,
			bHit ? FColor::Red : FColor::Green, false, DebugDuration);
		if (bHit)
		{
			DrawDebugPoint(World, OutHit.ImpactPoint, 10.f, FColor::Red, false, DebugDuration);
		}
	}
#endif

	return bHit;
}

bool UARPGTraceUtilities::SphereTraceByChannel(
	const UObject* WorldContextObject,
	FHitResult& OutHit,
	const FVector& Start,
	const FVector& End,
	float Radius,
	ECollisionChannel Channel,
	const TArray<AActor*>& ActorsToIgnore,
	bool bDrawDebug,
	float DebugDuration)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return false;

	FCollisionQueryParams Params = MakeQueryParams(ActorsToIgnore);
	FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
	bool bHit = World->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, Channel, Shape, Params);

#if ENABLE_DRAW_DEBUG
	if (bDrawDebug)
	{
		DrawDebugSphere(World, Start, Radius, 12, FColor::Green, false, DebugDuration);
		DrawDebugSphere(World, bHit ? OutHit.ImpactPoint : End, Radius, 12,
			bHit ? FColor::Red : FColor::Green, false, DebugDuration);
	}
#endif

	return bHit;
}

bool UARPGTraceUtilities::SphereTraceMultiByChannel(
	const UObject* WorldContextObject,
	TArray<FHitResult>& OutHits,
	const FVector& Start,
	const FVector& End,
	float Radius,
	ECollisionChannel Channel,
	const TArray<AActor*>& ActorsToIgnore,
	bool bDrawDebug,
	float DebugDuration)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return false;

	FCollisionQueryParams Params = MakeQueryParams(ActorsToIgnore);
	FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
	bool bHit = World->SweepMultiByChannel(OutHits, Start, End, FQuat::Identity, Channel, Shape, Params);

#if ENABLE_DRAW_DEBUG
	if (bDrawDebug)
	{
		DrawDebugSphere(World, Start, Radius, 12, FColor::Green, false, DebugDuration);
		DrawDebugSphere(World, End, Radius, 12, bHit ? FColor::Red : FColor::Green, false, DebugDuration);
	}
#endif

	return bHit;
}

bool UARPGTraceUtilities::BoxTraceByChannel(
	const UObject* WorldContextObject,
	FHitResult& OutHit,
	const FVector& Start,
	const FVector& End,
	const FVector& HalfExtents,
	const FRotator& Orientation,
	ECollisionChannel Channel,
	const TArray<AActor*>& ActorsToIgnore,
	bool bDrawDebug,
	float DebugDuration)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return false;

	FCollisionQueryParams Params = MakeQueryParams(ActorsToIgnore);
	FCollisionShape Shape = FCollisionShape::MakeBox(HalfExtents);
	bool bHit = World->SweepSingleByChannel(OutHit, Start, End, Orientation.Quaternion(), Channel, Shape, Params);

#if ENABLE_DRAW_DEBUG
	if (bDrawDebug)
	{
		DrawDebugBox(World, Start, HalfExtents, Orientation.Quaternion(), FColor::Green, false, DebugDuration);
		DrawDebugBox(World, bHit ? OutHit.ImpactPoint : End, HalfExtents, Orientation.Quaternion(),
			bHit ? FColor::Red : FColor::Green, false, DebugDuration);
	}
#endif

	return bHit;
}

bool UARPGTraceUtilities::CapsuleTraceByChannel(
	const UObject* WorldContextObject,
	FHitResult& OutHit,
	const FVector& Start,
	const FVector& End,
	float Radius,
	float HalfHeight,
	ECollisionChannel Channel,
	const TArray<AActor*>& ActorsToIgnore,
	bool bDrawDebug,
	float DebugDuration)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return false;

	FCollisionQueryParams Params = MakeQueryParams(ActorsToIgnore);
	FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	bool bHit = World->SweepSingleByChannel(OutHit, Start, End, FQuat::Identity, Channel, Shape, Params);

#if ENABLE_DRAW_DEBUG
	if (bDrawDebug)
	{
		DrawDebugCapsule(World, Start, HalfHeight, Radius, FQuat::Identity, FColor::Green, false, DebugDuration);
		DrawDebugCapsule(World, bHit ? OutHit.ImpactPoint : End, HalfHeight, Radius, FQuat::Identity,
			bHit ? FColor::Red : FColor::Green, false, DebugDuration);
	}
#endif

	return bHit;
}

bool UARPGTraceUtilities::SphereOverlapByChannel(
	const UObject* WorldContextObject,
	TArray<AActor*>& OutActors,
	const FVector& Center,
	float Radius,
	ECollisionChannel Channel,
	const TArray<AActor*>& ActorsToIgnore,
	bool bDrawDebug,
	float DebugDuration)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World) return false;

	OutActors.Empty();

	FCollisionQueryParams Params = MakeQueryParams(ActorsToIgnore);
	FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);

	TArray<FOverlapResult> Overlaps;
	bool bHit = World->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity, Channel, Shape, Params);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (AActor* Actor = Overlap.GetActor())
		{
			OutActors.AddUnique(Actor);
		}
	}

#if ENABLE_DRAW_DEBUG
	if (bDrawDebug)
	{
		DrawDebugSphere(World, Center, Radius, 16,
			bHit ? FColor::Red : FColor::Green, false, DebugDuration);
	}
#endif

	return OutActors.Num() > 0;
}

UARPGPhysicalMaterial* UARPGTraceUtilities::GetARPGPhysMaterial(const FHitResult& HitResult)
{
	return Cast<UARPGPhysicalMaterial>(HitResult.PhysMaterial.Get());
}

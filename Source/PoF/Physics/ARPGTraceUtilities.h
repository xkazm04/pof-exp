#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGTraceUtilities.generated.h"

/**
 * Static trace helper library with channel filtering and optional debug draw.
 * Wraps common line/sphere/box/capsule trace patterns for ARPG gameplay.
 */
UCLASS()
class POF_API UARPGTraceUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Line trace on a specific collision channel with optional debug draw.
	 * @return true if the trace hit something.
	 */
	UFUNCTION(BlueprintCallable, Category = "ARPG|Trace", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore"))
	static bool LineTraceByChannel(
		const UObject* WorldContextObject,
		FHitResult& OutHit,
		const FVector& Start,
		const FVector& End,
		ECollisionChannel Channel,
		const TArray<AActor*>& ActorsToIgnore,
		bool bDrawDebug = false,
		float DebugDuration = 2.0f);

	/**
	 * Sphere trace on a specific collision channel.
	 * @return true if the trace hit something.
	 */
	UFUNCTION(BlueprintCallable, Category = "ARPG|Trace", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore"))
	static bool SphereTraceByChannel(
		const UObject* WorldContextObject,
		FHitResult& OutHit,
		const FVector& Start,
		const FVector& End,
		float Radius,
		ECollisionChannel Channel,
		const TArray<AActor*>& ActorsToIgnore,
		bool bDrawDebug = false,
		float DebugDuration = 2.0f);

	/**
	 * Multi-sphere trace that returns all overlapping results (not just first).
	 * @return true if any hits were found.
	 */
	UFUNCTION(BlueprintCallable, Category = "ARPG|Trace", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore"))
	static bool SphereTraceMultiByChannel(
		const UObject* WorldContextObject,
		TArray<FHitResult>& OutHits,
		const FVector& Start,
		const FVector& End,
		float Radius,
		ECollisionChannel Channel,
		const TArray<AActor*>& ActorsToIgnore,
		bool bDrawDebug = false,
		float DebugDuration = 2.0f);

	/**
	 * Box trace on a specific collision channel.
	 * @return true if the trace hit something.
	 */
	UFUNCTION(BlueprintCallable, Category = "ARPG|Trace", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore"))
	static bool BoxTraceByChannel(
		const UObject* WorldContextObject,
		FHitResult& OutHit,
		const FVector& Start,
		const FVector& End,
		const FVector& HalfExtents,
		const FRotator& Orientation,
		ECollisionChannel Channel,
		const TArray<AActor*>& ActorsToIgnore,
		bool bDrawDebug = false,
		float DebugDuration = 2.0f);

	/**
	 * Capsule trace on a specific collision channel.
	 * @return true if the trace hit something.
	 */
	UFUNCTION(BlueprintCallable, Category = "ARPG|Trace", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore"))
	static bool CapsuleTraceByChannel(
		const UObject* WorldContextObject,
		FHitResult& OutHit,
		const FVector& Start,
		const FVector& End,
		float Radius,
		float HalfHeight,
		ECollisionChannel Channel,
		const TArray<AActor*>& ActorsToIgnore,
		bool bDrawDebug = false,
		float DebugDuration = 2.0f);

	/**
	 * Overlap sphere test — returns all actors overlapping a sphere on a given channel.
	 * @return true if any overlaps were found.
	 */
	UFUNCTION(BlueprintCallable, Category = "ARPG|Trace", meta = (WorldContext = "WorldContextObject", AutoCreateRefTerm = "ActorsToIgnore"))
	static bool SphereOverlapByChannel(
		const UObject* WorldContextObject,
		TArray<AActor*>& OutActors,
		const FVector& Center,
		float Radius,
		ECollisionChannel Channel,
		const TArray<AActor*>& ActorsToIgnore,
		bool bDrawDebug = false,
		float DebugDuration = 2.0f);

	/**
	 * Get the ARPG physical material from a hit result, or nullptr if none.
	 */
	UFUNCTION(BlueprintPure, Category = "ARPG|Trace")
	static class UARPGPhysicalMaterial* GetARPGPhysMaterial(const FHitResult& HitResult);
};

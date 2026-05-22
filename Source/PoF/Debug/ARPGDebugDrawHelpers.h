#pragma once

#include "CoreMinimal.h"
#include "DrawDebugHelpers.h"
#include "ARPGDebugDrawHelpers.generated.h"

/**
 * Static utility for ARPG debug visualization.
 * All draw calls are compiled out in Shipping builds via WITH_EDITOR / UE_BUILD_DEBUG guards.
 * Toggle at runtime: console var "ARPG.Debug.Draw" (0 = off, 1 = on).
 */
UCLASS()
class POF_API UARPGDebugDrawHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// =====================================================================
	// Master Toggle
	// =====================================================================

	/** Returns true if debug drawing is enabled (ARPG.Debug.Draw != 0). */
	static bool IsDebugDrawEnabled();

	// =====================================================================
	// Combat
	// =====================================================================

	/** Draw a weapon sweep trace between previous and current socket positions. */
	UFUNCTION(BlueprintCallable, Category = "ARPG|Debug", meta = (WorldContext = "WorldContextObject", DevelopmentOnly))
	static void DrawWeaponTrace(const UObject* WorldContextObject, const FVector& Start, const FVector& End,
		float Radius, bool bHit, float Duration = 0.05f);

	/** Draw ability range as a sphere around the caster. */
	UFUNCTION(BlueprintCallable, Category = "ARPG|Debug", meta = (WorldContext = "WorldContextObject", DevelopmentOnly))
	static void DrawAbilityRange(const UObject* WorldContextObject, const FVector& Center,
		float Radius, FLinearColor Color, float Duration = 2.f);

	/** Draw a hit impact point with a cross marker. */
	UFUNCTION(BlueprintCallable, Category = "ARPG|Debug", meta = (WorldContext = "WorldContextObject", DevelopmentOnly))
	static void DrawHitImpact(const UObject* WorldContextObject, const FVector& Location,
		const FVector& Normal, float Size = 15.f, float Duration = 1.f);

	// =====================================================================
	// AI
	// =====================================================================

	/** Draw an AI path as connected line segments with waypoint spheres. */
	UFUNCTION(BlueprintCallable, Category = "ARPG|Debug", meta = (WorldContext = "WorldContextObject", DevelopmentOnly))
	static void DrawAIPath(const UObject* WorldContextObject, const TArray<FVector>& PathPoints,
		FLinearColor Color, float Duration = 3.f);

	/** Draw perception range (sight/hearing) as a wireframe sphere. */
	UFUNCTION(BlueprintCallable, Category = "ARPG|Debug", meta = (WorldContext = "WorldContextObject", DevelopmentOnly))
	static void DrawPerceptionRange(const UObject* WorldContextObject, const FVector& Center,
		float SightRange, float HearingRange, float Duration = 0.5f);

	/** Draw an attack range cone for directional AI attacks. */
	UFUNCTION(BlueprintCallable, Category = "ARPG|Debug", meta = (WorldContext = "WorldContextObject", DevelopmentOnly))
	static void DrawAttackCone(const UObject* WorldContextObject, const FVector& Origin,
		const FVector& Direction, float Range, float HalfAngleDeg, FLinearColor Color, float Duration = 1.f);

	// =====================================================================
	// World / Spawning
	// =====================================================================

	/** Draw a spawn point marker (vertical arrow + label). */
	UFUNCTION(BlueprintCallable, Category = "ARPG|Debug", meta = (WorldContext = "WorldContextObject", DevelopmentOnly))
	static void DrawSpawnPoint(const UObject* WorldContextObject, const FVector& Location,
		const FString& Label, FLinearColor Color, float Duration = 5.f);

	/** Draw a box volume outline (e.g., trigger volumes, spawn areas). */
	UFUNCTION(BlueprintCallable, Category = "ARPG|Debug", meta = (WorldContext = "WorldContextObject", DevelopmentOnly))
	static void DrawBoxVolume(const UObject* WorldContextObject, const FVector& Center,
		const FVector& Extent, const FRotator& Rotation, FLinearColor Color, float Duration = 5.f);

	// =====================================================================
	// Collision / Physics
	// =====================================================================

	/** Draw a sphere overlap query result (the query sphere + hit locations). */
	UFUNCTION(BlueprintCallable, Category = "ARPG|Debug", meta = (WorldContext = "WorldContextObject", DevelopmentOnly))
	static void DrawOverlapSphere(const UObject* WorldContextObject, const FVector& Center,
		float Radius, const TArray<FVector>& HitLocations, float Duration = 1.f);
};

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGMaterialFunctionLibrary.generated.h"

class UMaterialInstanceDynamic;
class UMaterialParameterCollection;
class UMeshComponent;

/**
 * Blueprint function library for material-related utilities.
 * Provides UV manipulation, noise generation, blending helpers,
 * and convenience wrappers for common material operations.
 */
UCLASS()
class POF_API UARPGMaterialFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// =====================================================================
	// UV Manipulation
	// =====================================================================

	/** Compute world-aligned UV coordinates for triplanar mapping. */
	UFUNCTION(BlueprintPure, Category = "Materials|UV")
	static FVector2D WorldAlignedUV(const FVector& WorldPosition, const FVector& Normal, float Scale = 1.f);

	/**
	 * Compute triplanar blend weights from a world-space normal.
	 * @param WorldNormal  Surface normal in world space.
	 * @param Sharpness    Blend falloff exponent (higher = sharper transitions).
	 * @return XYZ weights that sum to 1.
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|UV")
	static FVector TriplanarBlendWeights(const FVector& WorldNormal, float Sharpness = 2.f);

	/** Rotate UV coordinates around (0.5, 0.5) by AngleDegrees. */
	UFUNCTION(BlueprintPure, Category = "Materials|UV")
	static FVector2D RotateUV(const FVector2D& UV, float AngleDegrees);

	/** Pan UVs over time. SpeedU/SpeedV are units per second. */
	UFUNCTION(BlueprintPure, Category = "Materials|UV")
	static FVector2D PanUV(const FVector2D& UV, float Time, float SpeedU, float SpeedV);

	/** Tile and offset UVs. */
	UFUNCTION(BlueprintPure, Category = "Materials|UV")
	static FVector2D TileOffsetUV(const FVector2D& UV, float TileU, float TileV, float OffsetU = 0.f, float OffsetV = 0.f);

	// =====================================================================
	// Noise / Procedural
	// =====================================================================

	/**
	 * Simple 2D value noise in [0..1] range.
	 * Useful for material variation, vertex displacement, etc.
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|Noise")
	static float SimpleNoise2D(float X, float Y);

	/**
	 * Fractal Brownian Motion (fBm) layered noise.
	 * @param Position   2D sample position.
	 * @param Octaves    Number of noise layers (1-8).
	 * @param Lacunarity Frequency multiplier per octave.
	 * @param Gain       Amplitude multiplier per octave (persistence).
	 * @return Noise value roughly in [0..1].
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|Noise")
	static float FBMNoise2D(FVector2D Position, int32 Octaves = 4, float Lacunarity = 2.f, float Gain = 0.5f);

	/**
	 * Voronoi cell distance for procedural patterns.
	 * @return Distance to nearest cell center (useful for cracking, cells).
	 */
	UFUNCTION(BlueprintPure, Category = "Materials|Noise")
	static float VoronoiDistance2D(FVector2D Position, float CellSize = 1.f);

	// =====================================================================
	// Blending
	// =====================================================================

	/** Height-based blend between two layers. Returns blend weight for layer B [0..1]. */
	UFUNCTION(BlueprintPure, Category = "Materials|Blend")
	static float HeightBlend(float HeightA, float HeightB, float BlendAlpha, float BlendContrast = 0.5f);

	/** Angle-based blend: returns 1 for surfaces facing UpDirection, 0 for perpendicular. */
	UFUNCTION(BlueprintPure, Category = "Materials|Blend")
	static float AngleBlend(const FVector& SurfaceNormal, const FVector& UpDirection, float Falloff = 1.f);

	/** Distance-based fade: returns 1 at Origin, fading to 0 at MaxDistance. */
	UFUNCTION(BlueprintPure, Category = "Materials|Blend")
	static float DistanceFade(const FVector& WorldPosition, const FVector& Origin, float MaxDistance);

	// =====================================================================
	// Convenience
	// =====================================================================

	/**
	 * Batch-set scalar parameters on a MID from a name-value map.
	 * More efficient than individual SetScalarParameterValue calls from BP.
	 */
	UFUNCTION(BlueprintCallable, Category = "Materials|Utility")
	static void SetScalarParameters(UMaterialInstanceDynamic* MID, const TMap<FName, float>& Parameters);

	/**
	 * Batch-set vector parameters on a MID.
	 */
	UFUNCTION(BlueprintCallable, Category = "Materials|Utility")
	static void SetVectorParameters(UMaterialInstanceDynamic* MID, const TMap<FName, FLinearColor>& Parameters);

	/**
	 * Create MIDs for all material slots on a mesh component, returning them as an array.
	 * Preserves original material assignments.
	 */
	UFUNCTION(BlueprintCallable, Category = "Materials|Utility")
	static TArray<UMaterialInstanceDynamic*> CreateDynamicMaterialsForMesh(UMeshComponent* MeshComp);

	/**
	 * Lerp a scalar parameter on a MID toward a target value.
	 * Call every frame for smooth transitions.
	 * @return The new current value after interpolation.
	 */
	UFUNCTION(BlueprintCallable, Category = "Materials|Utility")
	static float LerpScalarParameter(UMaterialInstanceDynamic* MID, FName ParamName, float TargetValue, float InterpSpeed, float DeltaTime);
};

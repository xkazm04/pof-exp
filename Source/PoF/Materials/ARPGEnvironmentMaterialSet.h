#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ARPGEnvironmentMaterialSet.generated.h"

/**
 * Defines a single environment surface material with PBR parameter overrides.
 * Used to stamp out material instances from a shared master material.
 */
USTRUCT(BlueprintType)
struct FARPGSurfaceMaterialDef
{
	GENERATED_BODY()

	/** Human-readable name (e.g., "Mossy Stone", "Dry Sand"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	FString Name;

	// ----- Textures -----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Textures")
	TSoftObjectPtr<UTexture2D> BaseColorMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Textures")
	TSoftObjectPtr<UTexture2D> NormalMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Textures")
	TSoftObjectPtr<UTexture2D> ORMMap; // Occlusion, Roughness, Metallic packed

	// ----- Scalar overrides -----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (ClampMin = "0", ClampMax = "1"))
	float Roughness = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (ClampMin = "0", ClampMax = "1"))
	float Metallic = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (ClampMin = "0", ClampMax = "1"))
	float Specular = 0.5f;

	/** UV tiling scale. Higher = more repetition, smaller detail. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (ClampMin = "0.01"))
	float UVScale = 1.f;

	/** Normal map intensity [0..2]. 0 = flat, 1 = default, 2 = exaggerated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters", meta = (ClampMin = "0", ClampMax = "2"))
	float NormalIntensity = 1.f;

	// ----- Color -----

	/** Base color tint multiplied with the texture. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parameters")
	FLinearColor ColorTint = FLinearColor::White;

	// ----- Detail / Variation -----

	/** Whether this surface supports detail texture at close range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detail")
	bool bUseDetailTexture = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detail", meta = (EditCondition = "bUseDetailTexture"))
	TSoftObjectPtr<UTexture2D> DetailNormalMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detail", meta = (EditCondition = "bUseDetailTexture", ClampMin = "0.01"))
	float DetailUVScale = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Detail", meta = (EditCondition = "bUseDetailTexture", ClampMin = "0", ClampMax = "1"))
	float DetailNormalIntensity = 0.5f;

	/** Whether to apply macro variation to break tiling at distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation")
	bool bUseMacroVariation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Variation", meta = (EditCondition = "bUseMacroVariation", ClampMin = "0", ClampMax = "1"))
	float MacroVariationAmount = 0.15f;
};

/**
 * Data asset containing a set of environment surface material definitions.
 * Designers populate this with surfaces for a biome or area (e.g., "Forest Set",
 * "Desert Set") and the level art pipeline uses it to configure material instances.
 */
UCLASS(BlueprintType)
class POF_API UARPGEnvironmentMaterialSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** The master/parent material these surfaces derive from. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material")
	TSoftObjectPtr<UMaterialInterface> BaseMasterMaterial;

	/** Descriptive name for this set (e.g., "Forest Floor", "Desert Rocks"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material")
	FString SetName;

	/** Surface material definitions in this set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surfaces")
	TArray<FARPGSurfaceMaterialDef> Surfaces;

	/**
	 * Create a dynamic material instance for a surface in this set.
	 * @param SurfaceIndex  Index into the Surfaces array.
	 * @param Outer         Outer object for the MID.
	 * @return Configured MID, or nullptr on failure.
	 */
	UFUNCTION(BlueprintCallable, Category = "Material")
	UMaterialInstanceDynamic* CreateSurfaceInstance(int32 SurfaceIndex, UObject* Outer) const;

	/** Find a surface by name (case-insensitive). Returns INDEX_NONE if not found. */
	UFUNCTION(BlueprintPure, Category = "Material")
	int32 FindSurfaceByName(const FString& SurfaceName) const;
};

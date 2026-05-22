#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ARPGBiomeDefinition.generated.h"

/** A single vegetation/foliage type within a biome. */
USTRUCT(BlueprintType)
struct FBiomeVegetationEntry
{
	GENERATED_BODY()

	/** Display name for this vegetation type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	FName EntryName;

	/** Mesh to instance for this vegetation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** Density of instances per 100x100 UE unit area. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Density", meta = (ClampMin = "0.01"))
	float DensityPer100Sq = 1.0f;

	/** Min/max slope angle (degrees) where this vegetation can grow. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Slope", meta = (ClampMin = "0", ClampMax = "90"))
	float MinSlopeAngle = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Slope", meta = (ClampMin = "0", ClampMax = "90"))
	float MaxSlopeAngle = 45.f;

	/** Altitude range (Z world units) where this vegetation can appear. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Altitude")
	float MinAltitude = -100000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Altitude")
	float MaxAltitude = 100000.f;

	/** Random scale range for instances. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Transform", meta = (ClampMin = "0.01"))
	float MinScale = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Transform", meta = (ClampMin = "0.01"))
	float MaxScale = 1.2f;

	/** Whether instances align to surface normal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Transform")
	bool bAlignToSurface = false;

	/** Random yaw rotation range (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Transform", meta = (ClampMin = "0", ClampMax = "360"))
	float RandomYawRange = 360.f;

	/** Minimum distance between instances of this type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Density", meta = (ClampMin = "0"))
	float MinSpacing = 50.f;

	/** Selection weight for mixed vegetation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = "0.01"))
	float SelectionWeight = 1.0f;

	/** Whether this vegetation blocks NavMesh (trees = yes, grass = no). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Navigation")
	bool bBlocksNavigation = false;
};

/** A scatter layer for non-vegetation props (rocks, debris, decals). */
USTRUCT(BlueprintType)
struct FBiomeScatterLayer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	FName LayerName;

	/** Meshes to randomly pick from for this layer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	TArray<TSoftObjectPtr<UStaticMesh>> Meshes;

	/** Density per 100x100 UE unit area. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "0.001"))
	float DensityPer100Sq = 0.5f;

	/** Random scale range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "0.01"))
	float MinScale = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "0.01"))
	float MaxScale = 1.5f;

	/** Min slope angle this scatter appears on. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "0", ClampMax = "90"))
	float MinSlopeAngle = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "0", ClampMax = "90"))
	float MaxSlopeAngle = 60.f;

	/** Minimum spacing between instances. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "0"))
	float MinSpacing = 100.f;

	/** Whether to align to surface normal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	bool bAlignToSurface = true;
};

/**
 * Data asset defining a biome's vegetation and scatter rules.
 * Used by ARPGVegetationScatter actors to procedurally populate areas.
 */
UCLASS(BlueprintType)
class POF_API UARPGBiomeDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Unique biome identifier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome")
	FName BiomeID;

	/** Display name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome")
	FText BiomeDisplayName;

	/** Vegetation entries for this biome. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome|Vegetation")
	TArray<FBiomeVegetationEntry> Vegetation;

	/** Non-vegetation scatter layers (rocks, debris, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome|Scatter")
	TArray<FBiomeScatterLayer> ScatterLayers;

	/** Global density multiplier for the entire biome. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome", meta = (ClampMin = "0.01"))
	float GlobalDensityMultiplier = 1.0f;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};

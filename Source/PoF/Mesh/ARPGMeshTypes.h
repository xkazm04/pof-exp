#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "ARPGMeshTypes.generated.h"

/** Mesh usage category — drives LOD, collision, and Nanite defaults. */
UENUM(BlueprintType)
enum class EARPGMeshCategory : uint8
{
	/** Small props: barrels, crates, pots */
	Prop,
	/** Large environment pieces: walls, floors, pillars */
	Architecture,
	/** Foliage and vegetation */
	Foliage,
	/** Characters and creatures (skeletal) */
	Character,
	/** Weapons and equipment */
	Equipment,
	/** VFX meshes (particles, decals) */
	VFX
};

/** Collision complexity preset. */
UENUM(BlueprintType)
enum class EARPGCollisionPreset : uint8
{
	/** No collision */
	None,
	/** Simple box/sphere/capsule — cheapest, good for small props */
	SimpleBox,
	/** Auto-convex decomposition — good balance for medium props */
	ConvexDecomposition,
	/** Per-poly collision — expensive, for walkable architecture */
	ComplexAsSimple,
	/** Custom UCX convex hulls imported from DCC */
	CustomConvex
};

/** LOD tier configuration. */
USTRUCT(BlueprintType)
struct POF_API FARPGLODTier
{
	GENERATED_BODY()

	/** Screen size threshold at which this LOD activates (0.0 = always visible). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ScreenSizeThreshold = 0.5f;

	/** Triangle reduction percentage for this LOD (0.0 = no reduction, 1.0 = full reduction). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReductionPercent = 0.5f;
};

/**
 * Mesh import configuration — defines how meshes should be imported and set up.
 * Used by the import pipeline and asset validation.
 */
USTRUCT(BlueprintType)
struct POF_API FARPGMeshImportConfig
{
	GENERATED_BODY()

	/** Uniform import scale (UE units). 1.0 = 1cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.001"))
	float ImportScale = 1.0f;

	/** Whether to combine meshes on import. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCombineMeshes = false;

	/** Whether to auto-generate lightmap UVs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bGenerateLightmapUVs = true;

	/** Lightmap resolution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "4", ClampMax = "4096"))
	int32 LightmapResolution = 64;

	/** Whether to remove degenerates on import. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRemoveDegenerates = true;

	/** Whether to build adjacency buffer (needed for tessellation). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bBuildAdjacencyBuffer = false;

	/** Whether to build reversed index buffer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bBuildReversedIndexBuffer = true;

	/** Whether to recompute normals. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRecomputeNormals = false;

	/** Whether to recompute tangents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRecomputeTangents = true;

	/** Auto-generate collision based on preset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EARPGCollisionPreset CollisionPreset = EARPGCollisionPreset::ConvexDecomposition;

	/** Max number of convex hulls for auto-decomposition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "64", EditCondition = "CollisionPreset == EARPGCollisionPreset::ConvexDecomposition"))
	int32 MaxConvexHulls = 4;

	/** Whether to enable Nanite for this mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableNanite = false;

	/** LOD tiers to auto-generate. Empty = no auto LOD. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FARPGLODTier> LODTiers;

	/** Category for this mesh type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EARPGMeshCategory Category = EARPGMeshCategory::Prop;
};

/**
 * Data table row for mesh asset registry.
 * Acts as a catalog of all mesh assets with metadata for runtime lookup.
 */
USTRUCT(BlueprintType)
struct POF_API FARPGMeshRegistryRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Display name for UI/tools. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	FText DisplayName;

	/** Soft reference to the static mesh asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	/** Soft reference to the skeletal mesh asset (if applicable). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	/** Category of this mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	EARPGMeshCategory Category = EARPGMeshCategory::Prop;

	/** Whether Nanite is enabled for this mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	bool bNaniteEnabled = false;

	/** Number of LODs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh", meta = (ClampMin = "1"))
	int32 LODCount = 1;

	/** Triangle count at LOD0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh", meta = (ClampMin = "0"))
	int32 TriangleCount = 0;

	/** Approximate bounding box dimensions in cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	FVector BoundsExtent = FVector::ZeroVector;

	/** Import configuration used for this mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	FARPGMeshImportConfig ImportConfig;
};

/**
 * Skeletal mesh import configuration — extends base config with skeleton-specific settings.
 */
USTRUCT(BlueprintType)
struct POF_API FARPGSkeletalMeshImportConfig
{
	GENERATED_BODY()

	/** Whether to import morph targets (blend shapes). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bImportMorphTargets = true;

	/** Whether to import mesh as skeletal even if no skeleton found. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bForceSkeletalImport = false;

	/** Whether to auto-create physics asset on import. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCreatePhysicsAsset = true;

	/** Minimum bone influence threshold — weights below this are culled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "0.1"))
	float MinBoneInfluence = 0.001f;

	/** Max bones per vertex for GPU skinning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "12"))
	int32 MaxBonesPerVertex = 4;

	/** Soft reference to the target skeleton for retargeting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<USkeleton> TargetSkeleton;

	/** LOD tiers for skeletal mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FARPGLODTier> LODTiers;
};

/**
 * Nanite foliage configuration for dense vegetation using Nanite assemblies.
 */
USTRUCT(BlueprintType)
struct POF_API FARPGNaniteFoliageConfig
{
	GENERATED_BODY()

	/** Enable Nanite for this foliage type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableNanite = true;

	/** Enable world-position-offset wind animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnableWPOWind = true;

	/** Wind intensity multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float WindIntensity = 1.0f;

	/** Nanite fallback LOD triangle percent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FallbackTrianglePercent = 0.5f;

	/** Maximum draw distance for foliage instances. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float CullDistance = 50000.f;

	/** Per-instance density scaling for GPU instancing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.01", ClampMax = "2.0"))
	float DensityScale = 1.0f;

	/** Minimum LOD to use (0 = full detail). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 MinLOD = 0;
};

/**
 * Data asset holding Nanite foliage presets — assign to foliage types for consistent wind/LOD behavior.
 */
UCLASS(BlueprintType)
class POF_API UARPGNaniteFoliageSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Named presets for foliage types. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Foliage")
	TMap<FName, FARPGNaniteFoliageConfig> FoliagePresets;

	/** Get preset by name, or return default if not found. */
	UFUNCTION(BlueprintPure, Category = "Foliage")
	FARPGNaniteFoliageConfig GetPreset(FName PresetName) const;

	/** Populate with standard ARPG foliage presets. */
	void InitializeFoliageDefaults();
};

/**
 * Data asset holding default import configurations per mesh category.
 */
UCLASS(BlueprintType)
class POF_API UARPGMeshImportSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Default import configs per category. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Import")
	TMap<EARPGMeshCategory, FARPGMeshImportConfig> CategoryDefaults;

	/** Default skeletal mesh import configs per category (Character, Equipment). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Import")
	TMap<EARPGMeshCategory, FARPGSkeletalMeshImportConfig> SkeletalDefaults;

	/** Get the default config for a category, or a sensible fallback. */
	UFUNCTION(BlueprintPure, Category = "Import")
	FARPGMeshImportConfig GetDefaultConfig(EARPGMeshCategory Category) const;

	/** Get the default skeletal config for a category. */
	UFUNCTION(BlueprintPure, Category = "Import")
	FARPGSkeletalMeshImportConfig GetDefaultSkeletalConfig(EARPGMeshCategory Category) const;

	/** Populate CategoryDefaults with sensible ARPG defaults. */
	void InitializeDefaults();
};

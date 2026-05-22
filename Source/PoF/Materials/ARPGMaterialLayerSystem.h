#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ARPGMaterialLayerSystem.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTexture2D;

/**
 * Blend mode for combining material layers.
 */
UENUM(BlueprintType)
enum class EARPGLayerBlendMode : uint8
{
	/** Linear interpolation based on blend weight. */
	LinearInterpolate,
	/** Height-based blending using heightmaps (e.g., rock showing through snow). */
	HeightBased,
	/** Angle-based blending using surface normal (e.g., snow on upward faces). */
	AngleBased,
	/** Vertex color channel drives the blend. */
	VertexColor
};

/**
 * Which vertex color channel to use for blending.
 */
UENUM(BlueprintType)
enum class EARPGVertexColorChannel : uint8
{
	Red,
	Green,
	Blue,
	Alpha
};

/**
 * Defines a single material layer — a reusable surface definition that can be
 * stacked and blended with other layers to compose complex multi-surface materials.
 *
 * Each layer holds PBR surface properties (textures + scalar overrides).
 * The Material Layer System composes them at runtime by writing blended
 * parameter values to a master material's MID.
 */
UCLASS(BlueprintType)
class POF_API UARPGMaterialLayer : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Human-readable layer name (e.g., "Base Rock", "Snow Cover", "Moss"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Layer")
	FString LayerName;

	// ----- Textures -----

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textures")
	TSoftObjectPtr<UTexture2D> BaseColor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textures")
	TSoftObjectPtr<UTexture2D> Normal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textures")
	TSoftObjectPtr<UTexture2D> ORM;

	/** Optional heightmap for height-based blending. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Textures")
	TSoftObjectPtr<UTexture2D> HeightMap;

	// ----- Scalar Properties -----

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (ClampMin = "0", ClampMax = "1"))
	float Roughness = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (ClampMin = "0", ClampMax = "1"))
	float Metallic = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (ClampMin = "0", ClampMax = "1"))
	float Specular = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (ClampMin = "0.01"))
	float UVScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties", meta = (ClampMin = "0", ClampMax = "2"))
	float NormalIntensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Properties")
	FLinearColor Tint = FLinearColor::White;
};

/**
 * Defines how two material layers are blended together.
 */
USTRUCT(BlueprintType)
struct FARPGLayerBlendConfig
{
	GENERATED_BODY()

	/** The layer to blend on top of the base. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend")
	TSoftObjectPtr<UARPGMaterialLayer> Layer;

	/** How to blend this layer with the one below. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend")
	EARPGLayerBlendMode BlendMode = EARPGLayerBlendMode::LinearInterpolate;

	/** Global blend weight [0..1]. Multiplied with the blend mode's computed weight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend", meta = (ClampMin = "0", ClampMax = "1"))
	float BlendWeight = 0.5f;

	/** Contrast for height-based blending. Higher = sharper transitions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend", meta = (ClampMin = "0.001", ClampMax = "1", EditCondition = "BlendMode == EARPGLayerBlendMode::HeightBased"))
	float HeightContrast = 0.2f;

	/** Falloff exponent for angle-based blending. Higher = tighter to the up vector. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend", meta = (ClampMin = "0.1", ClampMax = "10", EditCondition = "BlendMode == EARPGLayerBlendMode::AngleBased"))
	float AngleFalloff = 2.f;

	/** Which vertex color channel drives the blend when using VertexColor mode. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend", meta = (EditCondition = "BlendMode == EARPGLayerBlendMode::VertexColor"))
	EARPGVertexColorChannel VertexChannel = EARPGVertexColorChannel::Red;
};

/**
 * Data asset that defines a complete layered material composition.
 *
 * A layered material has a base layer plus N blend layers stacked on top.
 * At runtime, the system applies layer textures and parameters to a master
 * material's MID using indexed parameter names (Layer0_BaseColor, Layer1_BaseColor, etc.).
 *
 * This mirrors UE's Material Layer system conceptually but is driven entirely
 * from C++ data assets for maximum control and serialization.
 */
UCLASS(BlueprintType)
class POF_API UARPGMaterialLayerStack : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Descriptive name for this layer stack. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LayerStack")
	FString StackName;

	/** The master material that supports layered parameter inputs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LayerStack")
	TSoftObjectPtr<UMaterialInterface> MasterMaterial;

	/** The base (bottom) layer. Always fully opaque. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LayerStack")
	TSoftObjectPtr<UARPGMaterialLayer> BaseLayer;

	/** Blend layers stacked on top of the base, in order (bottom to top). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LayerStack")
	TArray<FARPGLayerBlendConfig> BlendLayers;

	/**
	 * Maximum number of layers the master material supports.
	 * Layers beyond this count are ignored at runtime.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LayerStack", meta = (ClampMin = "1", ClampMax = "8"))
	int32 MaxLayers = 4;

	/**
	 * Create a MID configured with all layer parameters.
	 * The master material must have parameters named:
	 *   Layer{N}_BaseColor, Layer{N}_Normal, Layer{N}_ORM,
	 *   Layer{N}_Roughness, Layer{N}_Metallic, Layer{N}_UVScale,
	 *   Layer{N}_BlendWeight, Layer{N}_BlendMode, etc.
	 * @param Outer  Outer object for the MID.
	 * @return Configured MID, or nullptr on failure.
	 */
	UFUNCTION(BlueprintCallable, Category = "LayerStack")
	UMaterialInstanceDynamic* CreateLayeredInstance(UObject* Outer) const;

	/** Get the total number of active layers (base + blend layers, capped by MaxLayers). */
	UFUNCTION(BlueprintPure, Category = "LayerStack")
	int32 GetActiveLayerCount() const;

	/**
	 * Update a specific blend layer's weight on an existing MID.
	 * Useful for runtime transitions (e.g., snow accumulation).
	 */
	UFUNCTION(BlueprintCallable, Category = "LayerStack")
	static void SetLayerBlendWeight(UMaterialInstanceDynamic* MID, int32 LayerIndex, float Weight);
};

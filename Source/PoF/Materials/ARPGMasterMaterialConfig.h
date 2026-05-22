#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ARPGMasterMaterialConfig.generated.h"

/**
 * Defines a single scalar parameter exposed by a master material.
 */
USTRUCT(BlueprintType)
struct FARPGMaterialScalarParam
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DefaultValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MinValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxValue = 1.f;

	/** Designer-facing tooltip for this parameter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Description;
};

/**
 * Defines a single vector/color parameter exposed by a master material.
 */
USTRUCT(BlueprintType)
struct FARPGMaterialVectorParam
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor DefaultValue = FLinearColor::White;

	/** If true, the editor shows a color picker instead of 4 float fields. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsColor = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Description;
};

/**
 * Defines a texture parameter slot on a master material.
 */
USTRUCT(BlueprintType)
struct FARPGMaterialTextureParam
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture> DefaultTexture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Description;
};

/**
 * Data asset that describes the parameter layout of a master material.
 * Designers create one of these per master material to document and
 * configure which parameters are available for material instances.
 */
UCLASS(BlueprintType)
class POF_API UARPGMasterMaterialConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Reference to the actual master material asset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material")
	TSoftObjectPtr<UMaterialInterface> MasterMaterial;

	/** Human-readable name for the material (e.g., "Character PBR", "Environment Opaque"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material")
	FString MaterialName;

	/** All scalar parameters this master material exposes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters")
	TArray<FARPGMaterialScalarParam> ScalarParameters;

	/** All vector/color parameters this master material exposes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters")
	TArray<FARPGMaterialVectorParam> VectorParameters;

	/** All texture parameters this master material exposes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Parameters")
	TArray<FARPGMaterialTextureParam> TextureParameters;

	/**
	 * Create a dynamic material instance from this master material config,
	 * initialized to all default parameter values.
	 * @param Outer  The outer object (typically the owning actor or component).
	 * @return A new MID, or nullptr if MasterMaterial is not loaded.
	 */
	UFUNCTION(BlueprintCallable, Category = "Material")
	UMaterialInstanceDynamic* CreateConfiguredInstance(UObject* Outer) const;
};

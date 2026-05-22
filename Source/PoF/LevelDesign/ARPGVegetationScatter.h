#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGVegetationScatter.generated.h"

class UARPGBiomeDefinition;
class UBoxComponent;
class UHierarchicalInstancedStaticMeshComponent;

/**
 * Procedural vegetation scatter actor.
 * Place in the world, assign a BiomeDefinition, and call GenerateVegetation.
 *
 * Uses HierarchicalInstancedStaticMeshComponents for efficient rendering.
 * Respects slope, altitude, and spacing rules from the biome definition.
 * Traces against world geometry to find valid placement points.
 */
UCLASS()
class POF_API AARPGVegetationScatter : public AActor
{
	GENERATED_BODY()

public:
	AARPGVegetationScatter();

	/** Generate vegetation instances within the scatter bounds. */
	UFUNCTION(BlueprintCallable, Category = "LevelDesign|Vegetation")
	void GenerateVegetation();

	/** Clear all generated instances. */
	UFUNCTION(BlueprintCallable, Category = "LevelDesign|Vegetation")
	void ClearVegetation();

	/** Regenerate with a new seed. */
	UFUNCTION(BlueprintCallable, Category = "LevelDesign|Vegetation")
	void RegenerateWithSeed(int32 NewSeed);

	/** Get total instance count across all HISM components. */
	UFUNCTION(BlueprintPure, Category = "LevelDesign|Vegetation")
	int32 GetTotalInstanceCount() const;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Biome definition driving vegetation rules. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	TObjectPtr<UARPGBiomeDefinition> BiomeDefinition;

	/** Scatter area bounds. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vegetation")
	TObjectPtr<UBoxComponent> ScatterBounds;

	/** Random seed for reproducible scatter. 0 = random. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Generation")
	int32 RandomSeed = 0;

	/** Density multiplier (stacks with biome's GlobalDensityMultiplier). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Generation", meta = (ClampMin = "0.01"))
	float LocalDensityMultiplier = 1.0f;

	/** Whether to generate in editor on construction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Generation")
	bool bGenerateInEditor = false;

	/** Whether to generate on BeginPlay at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Generation")
	bool bGenerateOnBeginPlay = true;

	/** Trace channel for finding ground surface. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Generation")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** Exclusion volumes — no vegetation is placed inside these. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Exclusion")
	TArray<TObjectPtr<AActor>> ExclusionVolumes;

	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HISMComponents;

	FRandomStream RNG;

	/** Create or find an HISM component for a given mesh. */
	UHierarchicalInstancedStaticMeshComponent* GetOrCreateHISM(UStaticMesh* Mesh, bool bBlocksNav);

	/** Check if a point is inside any exclusion volume. */
	bool IsInExclusionZone(const FVector& Point) const;

	/** Get the slope angle at a point from its surface normal. */
	static float GetSlopeAngleFromNormal(const FVector& Normal);

	/** Check if a proposed instance is far enough from existing instances. */
	bool CheckMinSpacing(const UHierarchicalInstancedStaticMeshComponent* HISM, const FVector& Point, float MinSpacing) const;
};

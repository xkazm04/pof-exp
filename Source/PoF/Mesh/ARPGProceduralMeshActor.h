#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "ARPGProceduralMeshActor.generated.h"

/**
 * Actor with a ProceduralMeshComponent for runtime mesh generation.
 * Provides helper functions for common procedural shapes used in gameplay:
 * platforms, ramps, walls, and custom vertex data.
 */
UCLASS(BlueprintType, Blueprintable)
class POF_API AARPGProceduralMeshActor : public AActor
{
	GENERATED_BODY()

public:
	AARPGProceduralMeshActor();

	/** The procedural mesh component. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UProceduralMeshComponent> ProceduralMesh;

	/** Material to apply to generated mesh sections. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	TObjectPtr<UMaterialInterface> MeshMaterial;

	/** Whether to generate collision for the procedural mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	bool bCreateCollision = true;

	/**
	 * Generate a flat rectangular platform.
	 * @param Width Width in X (cm)
	 * @param Depth Depth in Y (cm)
	 * @param Height Thickness in Z (cm)
	 * @param SubdivisionsX Number of X subdivisions for vertex density
	 * @param SubdivisionsY Number of Y subdivisions for vertex density
	 */
	UFUNCTION(BlueprintCallable, Category = "Mesh|Generation")
	void GeneratePlatform(float Width = 200.f, float Depth = 200.f, float Height = 20.f, int32 SubdivisionsX = 1, int32 SubdivisionsY = 1);

	/**
	 * Generate a ramp/slope mesh.
	 * @param Width Width in X (cm)
	 * @param Depth Depth in Y (cm)
	 * @param Height Height at the high end in Z (cm)
	 */
	UFUNCTION(BlueprintCallable, Category = "Mesh|Generation")
	void GenerateRamp(float Width = 200.f, float Depth = 400.f, float Height = 200.f);

	/**
	 * Generate a vertical wall segment.
	 * @param Width Width in X (cm)
	 * @param Height Height in Z (cm)
	 * @param Thickness Depth in Y (cm)
	 */
	UFUNCTION(BlueprintCallable, Category = "Mesh|Generation")
	void GenerateWall(float Width = 400.f, float Height = 300.f, float Thickness = 20.f);

	/**
	 * Generate a cylinder (pillar/column).
	 * @param Radius Radius in cm
	 * @param Height Height in cm
	 * @param Segments Number of radial segments
	 */
	UFUNCTION(BlueprintCallable, Category = "Mesh|Generation")
	void GenerateCylinder(float Radius = 50.f, float Height = 300.f, int32 Segments = 16);

	/**
	 * Generate mesh from raw vertex data.
	 * @param SectionIndex Mesh section to create/update
	 * @param Vertices Vertex positions
	 * @param Triangles Triangle indices
	 * @param Normals Per-vertex normals (optional, auto-calculated if empty)
	 * @param UVs Per-vertex UV coordinates
	 * @param VertexColors Per-vertex colors (optional)
	 */
	UFUNCTION(BlueprintCallable, Category = "Mesh|Generation")
	void GenerateCustomMesh(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles,
		const TArray<FVector>& Normals, const TArray<FVector2D>& UVs, const TArray<FColor>& VertexColors);

	/** Clear all mesh sections. */
	UFUNCTION(BlueprintCallable, Category = "Mesh|Generation")
	void ClearMesh();

protected:
	void ApplyMaterial(int32 SectionIndex);
};

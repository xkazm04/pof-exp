#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ARPGMaterialPerformanceManager.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMeshComponent;
class UStaticMeshComponent;

/**
 * Quality tier for material LOD and feature gating.
 */
UENUM(BlueprintType)
enum class EARPGMaterialQuality : uint8
{
	Low,
	Medium,
	High,
	Ultra
};

/**
 * Per-material complexity profile used for budgeting.
 */
USTRUCT(BlueprintType)
struct FARPGMaterialComplexity
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	FName MaterialName;

	/** Estimated texture sample count. */
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	int32 TextureSamples = 0;

	/** Number of shader instructions (approximate). */
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	int32 InstructionCount = 0;

	/** Whether the material uses translucency. */
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	bool bIsTranslucent = false;

	/** Whether the material uses world position offset. */
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	bool bUsesWPO = false;

	/** Whether the material uses tessellation/displacement. */
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	bool bUsesDisplacement = false;

	/** Computed complexity score (higher = more expensive). */
	UPROPERTY(BlueprintReadOnly, Category = "Performance")
	float ComplexityScore = 0.f;
};

/**
 * Configuration for distance-based material quality reduction.
 */
USTRUCT(BlueprintType)
struct FARPGMaterialLODConfig
{
	GENERATED_BODY()

	/** Distance from camera at which to switch to this LOD level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
	float SwitchDistance = 0.f;

	/** Quality tier for this LOD level. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
	EARPGMaterialQuality Quality = EARPGMaterialQuality::High;

	/** Whether to disable detail textures at this LOD. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
	bool bDisableDetailTextures = false;

	/** Whether to disable macro variation at this LOD. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
	bool bDisableMacroVariation = false;

	/** Whether to disable normal maps at this LOD. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
	bool bDisableNormalMap = false;

	/** UV scale multiplier (reduce detail at distance). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD", meta = (ClampMin = "0.1"))
	float UVScaleMultiplier = 1.f;
};

/**
 * Tracks a registered mesh for material instancing and LOD management.
 */
USTRUCT()
struct FARPGManagedMesh
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<UMeshComponent> MeshComp;

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> SharedMIDs;

	/** Current LOD level applied. INDEX_NONE = not yet evaluated. */
	int32 CurrentLODLevel = INDEX_NONE;
};

/**
 * World subsystem that manages material performance:
 *   - Material instancing (shared MIDs for identical meshes)
 *   - Distance-based material LOD (reduce features at distance)
 *   - Material complexity profiling
 *   - Quality tier management
 *
 * Register meshes with RegisterMesh() and the subsystem handles
 * shared instancing and LOD switching automatically each frame.
 */
UCLASS()
class POF_API UARPGMaterialPerformanceManager : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	// ----- Quality Tier -----

	/** Set the global material quality tier. Affects all registered meshes. */
	UFUNCTION(BlueprintCallable, Category = "Materials|Performance")
	void SetQualityTier(EARPGMaterialQuality Quality);

	UFUNCTION(BlueprintPure, Category = "Materials|Performance")
	EARPGMaterialQuality GetQualityTier() const { return CurrentQuality; }

	// ----- Mesh Registration -----

	/**
	 * Register a mesh component for managed instancing and LOD.
	 * Creates shared MIDs if another mesh with the same materials is already registered.
	 */
	UFUNCTION(BlueprintCallable, Category = "Materials|Performance")
	void RegisterMesh(UMeshComponent* MeshComp);

	/** Unregister a mesh. Its MIDs are released. */
	UFUNCTION(BlueprintCallable, Category = "Materials|Performance")
	void UnregisterMesh(UMeshComponent* MeshComp);

	// ----- LOD Configuration -----

	/** LOD distance thresholds. Sorted by SwitchDistance ascending. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|LOD")
	TArray<FARPGMaterialLODConfig> LODLevels;

	/** How often (seconds) to re-evaluate LOD levels. Lower = more responsive, higher = cheaper. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|LOD", meta = (ClampMin = "0.05"))
	float LODUpdateInterval = 0.25f;

	// ----- Profiling -----

	/**
	 * Profile a material's complexity.
	 * Reads material properties to estimate shader cost.
	 */
	UFUNCTION(BlueprintCallable, Category = "Materials|Performance")
	FARPGMaterialComplexity ProfileMaterial(UMaterialInterface* Material) const;

	/** Get the number of unique shared material instances currently in use. */
	UFUNCTION(BlueprintPure, Category = "Materials|Performance")
	int32 GetSharedInstanceCount() const { return SharedMIDPool.Num(); }

	/** Get the number of registered meshes. */
	UFUNCTION(BlueprintPure, Category = "Materials|Performance")
	int32 GetRegisteredMeshCount() const { return ManagedMeshes.Num(); }

private:
	EARPGMaterialQuality CurrentQuality = EARPGMaterialQuality::High;

	/** Pool of shared MIDs keyed by source material path. */
	TMap<FName, TObjectPtr<UMaterialInstanceDynamic>> SharedMIDPool;

	TArray<FARPGManagedMesh> ManagedMeshes;

	float TimeSinceLastLODUpdate = 0.f;

	void UpdateLODLevels();
	int32 DetermineLODLevel(float DistanceToCamera) const;
	void ApplyLODToMesh(FARPGManagedMesh& Managed, int32 LODLevel);
	UMaterialInstanceDynamic* GetOrCreateSharedMID(UMaterialInterface* SourceMat);
	void CleanupStaleEntries();
};

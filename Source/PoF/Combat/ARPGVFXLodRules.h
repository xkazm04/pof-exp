#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGVFXLodRules.generated.h"

/**
 * VFX Niagara LOD-band rules (catalog pipeline: vfx). Distance-based LOD selection:
 * LOD0 full (0–15 m), LOD1 medium (15–35 m), LOD2 culled (>35 m). Mirrors
 * src/lib/catalog/pipelines/vfx.ts. GPU-budget/Niagara-render is the RHI visual concern;
 * this encodes the deterministic distance→LOD selection.
 */
UENUM(BlueprintType)
enum class EARPGVFXLod : uint8
{
	Lod0Full,
	Lod1Medium,
	Lod2Culled
};

UCLASS()
class POF_API UARPGVFXLodRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr float Lod0MaxMeters = 15.f;
	static constexpr float Lod1MaxMeters = 35.f;

	/** LOD for a viewer distance in metres: <15 → LOD0, 15..35 → LOD1, >35 → culled. */
	UFUNCTION(BlueprintPure, Category = "VFX|LOD")
	static EARPGVFXLod LodForDistanceMeters(float DistanceMeters)
	{
		if (DistanceMeters < Lod0MaxMeters) return EARPGVFXLod::Lod0Full;
		if (DistanceMeters <= Lod1MaxMeters) return EARPGVFXLod::Lod1Medium;
		return EARPGVFXLod::Lod2Culled;
	}
};

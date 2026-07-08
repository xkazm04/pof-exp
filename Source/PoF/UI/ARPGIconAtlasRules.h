#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGIconAtlasRules.generated.h"

/**
 * HUD icon-atlas packing rules (catalog pipeline: icon-sets). A 4096x4096 BC7 atlas
 * of 256px cells → a 16x16 grid = 256 cells; 224 allocated + 32 reserved. Mip chain
 * floors at 32px. Mirrors src/lib/catalog/pipelines/icon-sets.ts (atlasBudget).
 */
UCLASS()
class POF_API UARPGIconAtlasRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr int32 AtlasSizePx = 4096;
	static constexpr int32 CellSizePx = 256;
	static constexpr int32 AllocatedSlots = 224;
	static constexpr int32 ReservedSlots = 32;
	static constexpr int32 MipFloorPx = 32;

	/** Cells per axis = AtlasSize / CellSize (16). */
	UFUNCTION(BlueprintPure, Category = "UI|IconAtlas")
	static int32 CellsPerAxis() { return AtlasSizePx / CellSizePx; }

	/** Total cells = CellsPerAxis^2 (256). */
	UFUNCTION(BlueprintPure, Category = "UI|IconAtlas")
	static int32 TotalCells() { return CellsPerAxis() * CellsPerAxis(); }

	/** Number of mip levels from a cell (256px) down to the 32px floor inclusive. */
	UFUNCTION(BlueprintPure, Category = "UI|IconAtlas")
	static int32 MipCount()
	{
		int32 Levels = 1;
		for (int32 Size = CellSizePx; Size > MipFloorPx; Size >>= 1) { ++Levels; }
		return Levels; // 256,128,64,32 → 4
	}
};

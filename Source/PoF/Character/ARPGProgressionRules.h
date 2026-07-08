#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGProgressionRules.generated.h"

/**
 * Hero XP-curve rules (catalog pipeline: progression-curves). Geometric curve
 * xpToNext(L) = 100 * 1.08^L, soft-capped at level 90 (growth clamps), hard cap 100.
 * The single C++ source of truth that CT_XPRequirements is generated from and that
 * the L3 gate (VSProgressionCurveTest) asserts. Mirrors src/lib/catalog/pipelines/
 * progression-curves.ts.
 */
UCLASS()
class POF_API UARPGProgressionRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr int32 XpBase = 100;
	static constexpr int32 SoftCapLevel = 90;
	static constexpr int32 HardCapLevel = 100;

	/** XP required to advance FROM the given level. Growth is clamped at SoftCapLevel,
	 *  so xpToNext(91..100) == xpToNext(90). */
	UFUNCTION(BlueprintPure, Category = "Progression")
	static int32 XpToNextLevel(int32 Level);
};

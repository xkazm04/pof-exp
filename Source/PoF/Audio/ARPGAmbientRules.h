#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGAmbientRules.generated.h"

/**
 * Ambient soundscape config rules (catalog pipeline: ambient). The forest bed plays at
 * −6 dB, three detail-loop emitters run within their radii, and the ember-drift one-shot
 * fires on a ≤20 s interval. Mirrors src/lib/catalog/pipelines/ambient.ts. Actual PIE
 * playback/spatialization is the runtime concern; this encodes the deterministic config.
 */
UCLASS()
class POF_API UARPGAmbientRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr float BedGainDb = -6.f;
	static constexpr int32 DetailEmitterCount = 3;
	static constexpr float EmberMaxIntervalSeconds = 20.f;

	/** True when a requested bed gain matches the calibrated −6 dB reference (±0.01). */
	UFUNCTION(BlueprintPure, Category = "Audio|Ambient")
	static bool IsBedGainCalibrated(float GainDb) { return FMath::IsNearlyEqual(GainDb, BedGainDb, 0.01f); }

	/** The ember one-shot must fire within the first interval window. */
	UFUNCTION(BlueprintPure, Category = "Audio|Ambient")
	static bool EmberFiresWithinFirstInterval(float ElapsedSeconds) { return ElapsedSeconds <= EmberMaxIntervalSeconds; }
};

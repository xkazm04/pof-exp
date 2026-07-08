#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGInputRebindRules.generated.h"

/**
 * Input-rebind rules (catalog pipeline: input-schemes). A rebind is rejected when the
 * new key already maps to another action (FindConflictingAction — no silent overwrite);
 * analog deadzone is clamped to a valid range. Mirrors src/lib/catalog/pipelines/
 * input-schemes.ts. Persistence (SaveGame round-trip) is the runtime concern; this
 * encodes the deterministic conflict + deadzone rules.
 */
UCLASS()
class POF_API UARPGInputRebindRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr float MaxDeadzone = 0.95f;

	/** True when NewKey is already bound to another action — the rebind must be rejected. */
	UFUNCTION(BlueprintPure, Category = "Input|Rebind")
	static bool WouldConflict(const TArray<FName>& ExistingBoundKeys, FName NewKey)
	{ return ExistingBoundKeys.Contains(NewKey); }

	/** Clamp a requested deadzone into the valid [0, 0.95] range (applied immediately). */
	UFUNCTION(BlueprintPure, Category = "Input|Rebind")
	static float ClampDeadzone(float Requested) { return FMath::Clamp(Requested, 0.f, MaxDeadzone); }
};

// Copyright PoF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PoFScenarioRunner.generated.h"

/** One injected input over a time window in a scenario. */
USTRUCT(BlueprintType)
struct FPoFTimedInput
{
    GENERATED_BODY()

    /** Input Action asset path, e.g. /Game/Input/Actions/IA_Move. */
    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    FString ActionPath;

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    FVector2D Value = FVector2D::ZeroVector;

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    float StartSeconds = 0.f;

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    float DurationSeconds = 1.f;
};

/**
 * SP1 RunScenario harness: opens PIE at a map, possesses player 0, injects timed
 * EnhancedInput actions while ticking deterministically, and leaves the PIE world
 * live so pose/frame observers (EvaluatePose mode=pie, CaptureFrame) can read it.
 */
UCLASS()
class POFEDITOR_API UPoFScenarioRunner : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Returns true if PIE started and a pawn was possessed. PIE is left running.
     *  If ScreenshotPath is non-empty, a high-res screenshot of the live PIE game
     *  view is captured near the end of the tick loop (the deterministic, correct
     *  T4 moment — the character is posed and PIE is active). */
    UFUNCTION(BlueprintCallable, Category = "PoF|Scenario", meta = (ScriptMethod))
    static bool RunScenario(const FString& MapPath, const TArray<FPoFTimedInput>& Inputs,
        float TotalSeconds, const FString& ScreenshotPath);

    /** Stop the active PIE session. */
    UFUNCTION(BlueprintCallable, Category = "PoF|Scenario", meta = (ScriptMethod))
    static void StopScenario();
};

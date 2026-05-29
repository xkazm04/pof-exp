// Copyright PoF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PoFScenarioRunner.generated.h"

/** One injected input over a time window.
 *  If Key is set (e.g. "W"), it is injected as a REAL simulated hardware key through
 *  the PlayerController — so the InputMappingContext bindings + modifiers are actually
 *  exercised (catches "only D works" class of bugs). Otherwise ActionPath + Value is
 *  injected at the action level (bypasses bindings; legacy behaviour). */
USTRUCT(BlueprintType)
struct FPoFTimedInput
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    FString ActionPath;

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    FVector2D Value = FVector2D::ZeroVector;

    /** Key name (e.g. "W","A","S","D","SpaceBar"). When set, injected as a real key. */
    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    FString Key;

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    float StartSeconds = 0.f;

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    float DurationSeconds = 1.f;
};

/** One time-sampled observation taken INSIDE the PIE tick loop (pose is freshly
 *  evaluated, not the reference pose). ArmDroop ~0deg = arms horizontal (T-POSE);
 *  ~50-80deg = arms hanging down. */
USTRUCT(BlueprintType)
struct FPoFPoseSample
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    float Time = 0.f;

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    float Speed = 0.f;

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    float ArmDroopLeftDeg = 0.f;

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    float ArmDroopRightDeg = 0.f;

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    bool PoseValid = false;

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    FString Frame;
};

USTRUCT(BlueprintType)
struct FPoFScenarioResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    bool Started = false;

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    FString Error;

    UPROPERTY(BlueprintReadWrite, Category = "PoF|Scenario")
    TArray<FPoFPoseSample> Samples;
};

/**
 * PIE scenario harness. RunScenario is the legacy single-capture path. RunScenarioEx
 * is the reliable verification harness: it injects timed inputs (real keys when Key is
 * set) and, at NumSamples evenly-spaced moments, reads the EVALUATED pose (after
 * RefreshBoneTransforms — the real animated bones, not the ref pose), the
 * location/speed, and a captured frame — all INSIDE the deterministic tick loop. This
 * is what makes "is it walking / is it T-posing / does WASD move it" answerable from
 * ground truth instead of from proxies.
 */
UCLASS()
class POFEDITOR_API UPoFScenarioRunner : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "PoF|Scenario")
    static bool RunScenario(const FString& MapPath, const TArray<FPoFTimedInput>& Inputs,
        float TotalSeconds, const FString& ScreenshotPath);

    UFUNCTION(BlueprintCallable, Category = "PoF|Scenario")
    static FPoFScenarioResult RunScenarioEx(const FString& MapPath, const TArray<FPoFTimedInput>& Inputs,
        float TotalSeconds, const FString& FrameDir, int32 NumSamples);

    UFUNCTION(BlueprintCallable, Category = "PoF|Scenario")
    static void StopScenario();
};

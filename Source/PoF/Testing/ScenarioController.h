#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ScenarioController.generated.h"

class APawn;
class USkeletalMeshComponent;
class UAnimMontage;
class FJsonObject;
class FJsonValue;

/** One injected input over a time window. Key => real simulated key through the
 *  PlayerController (exercises IMC bindings). Else ActionPath+Value => action-level. */
USTRUCT()
struct FScnInput
{
    GENERATED_BODY()
    FString Key;
    FString ActionPath;
    FVector2D Value = FVector2D::ZeroVector;
    /** Non-input event fired at Start: "activate_ability" => ASC TryActivateAbilitiesByTag(EventArg).
     *  Bypasses input fidelity gaps to test the ability's effect directly (L3 for GAS). */
    FString Event;
    FString EventArg;
    float Start = 0.f;
    float Duration = 1.f;
};

/**
 * Runtime scenario controller. Ticks in the REAL game loop (standalone -game or
 * naturally-ticked PIE) — never editor-automation World::Tick. Armed only when the
 * process is launched with -PoFScenario=<json>; otherwise fully dormant (no overhead in
 * normal play). It waits for a possessed pawn, drives a timed input timeline through the
 * real input path, samples observations (evaluated pose / location / velocity) and a frame
 * at time-based checkpoints, writes observations.json + frame_NN.png + DONE, and (in a
 * standalone session) requests exit. This is the faithful execution engine behind the
 * Observation Spine — what makes "does the mechanic work" answerable from ground truth.
 */
UCLASS()
class POF_API UScenarioController : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return bArmed && !bDone; }
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UScenarioController, STATGROUP_Tickables);
    }
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override
    {
        return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
    }

private:
    bool bArmed = false;
    bool bStarted = false;
    bool bDone = false;
    bool bIsStandalone = false;

    float PreElapsed = 0.f;   // time since BeginPlay, before the scenario starts
    float SettleTime = 0.5f;
    float ScnTime = 0.f;      // scenario clock (starts when pawn possessed + settled)
    float TotalSeconds = 3.f;
    int32 NumSamples = 6;
    int32 NextSample = 0;

    FString OutDir;
    FString PlayAnim;  // optional: force single-node play of this anim asset at Begin (manipulation/diagnostic)
    TArray<FScnInput> Inputs;
    TArray<float> SampleTimes;
    TArray<bool> WasActive;  // per-input edge tracking for proper held-key semantics
    TArray<TSharedPtr<FJsonValue>> SamplesJson;

    // Dense per-tick motion trace: capsule movement vs animation root motion, speed, yaw,
    // active montage + position. Lets movement be analysed as a continuous profile (accel
    // curves, exact distance/timing) and exposes foot-slide (capsule delta != root delta).
    TArray<TSharedPtr<FJsonValue>> TraceJson;
    FVector PrevTraceLoc = FVector::ZeroVector;
    bool bHavePrevTrace = false;
    TObjectPtr<UAnimMontage> PrevMontage = nullptr;
    float PrevMontagePos = -1.f;

    bool LoadScenario(const FString& Path);
    void Begin();
    void ApplyInputs();
    void DoSample(int32 Idx);
    void RecordTrace(float DeltaTime);
    void Finish();

    APawn* GetPawn() const;
    USkeletalMeshComponent* GetMesh() const;
    FString CaptureFrame(int32 Idx);
};

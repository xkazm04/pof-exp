#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PofSnapshotCapture.generated.h"

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofCameraPreset
{
    GENERATED_BODY()

    UPROPERTY() FString Id;
    UPROPERTY() FString Name;
    UPROPERTY() FString Description;
    UPROPERTY() FVector Location = FVector::ZeroVector;
    UPROPERTY() FRotator Rotation = FRotator::ZeroRotator;
    UPROPERTY() float FOV = 60.0f;
    UPROPERTY() FIntPoint Resolution = FIntPoint(1920, 1080);
    UPROPERTY() FString MapPath;
    UPROPERTY() bool RequiresPIE = false;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofSnapshotResult
{
    GENERATED_BODY()

    UPROPERTY() FString PresetId;
    UPROPERTY() FString FilePath;
    UPROPERTY() int32 Width = 0;
    UPROPERTY() int32 Height = 0;
    UPROPERTY() FDateTime Timestamp;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofDiffResult
{
    GENERATED_BODY()

    UPROPERTY() FString PresetId;
    UPROPERTY() FString Status;
    UPROPERTY() float DiffPercentage = 0.0f;
    UPROPERTY() float MaxPixelDiff = 0.0f;
    UPROPERTY() int32 DiffPixelCount = 0;
    UPROPERTY() int32 TotalPixelCount = 0;
    UPROPERTY() bool Passed = true;
    UPROPERTY() FString BaselinePath;
    UPROPERTY() FString CapturePath;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofSnapshotDiffReport
{
    GENERATED_BODY()

    UPROPERTY() FString GeneratedAt;
    UPROPERTY() float DiffThreshold = 0.02f;
    UPROPERTY() FString OverallStatus;
    UPROPERTY() TArray<FPofDiffResult> Results;
    UPROPERTY() int32 TotalPresets = 0;
    UPROPERTY() int32 PassedCount = 0;
    UPROPERTY() int32 FailedCount = 0;
};

UCLASS()
class PILLARSOFFORTUNEBRIDGE_API UPofSnapshotCapture : public UObject
{
    GENERATED_BODY()

public:
    void CapturePreset(const FPofCameraPreset& Preset, bool bSaveAsBaseline = false);
    FPofDiffResult CompareWithBaseline(const FString& PresetId, float DiffThreshold = 0.02f);
    FPofSnapshotDiffReport GenerateDiffReport(const TArray<FString>& PresetIds, float DiffThreshold);
    TArray<FPofCameraPreset> LoadCameraPresets() const;

    const TArray<FPofSnapshotResult>& GetResults() const { return SnapshotResults; }

private:
    FString GetPofDirectory() const;
    FString GetLatestSnapshotPath(const FString& PresetId) const;

    TArray<FPofSnapshotResult> SnapshotResults;
};

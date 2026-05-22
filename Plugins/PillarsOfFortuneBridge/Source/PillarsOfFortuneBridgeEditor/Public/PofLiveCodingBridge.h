#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PofLiveCodingBridge.generated.h"

UENUM()
enum class EPofCompileStatus : uint8
{
    Idle,
    Compiling,
    Success,
    Failed,
    Error,
    Timeout
};

UENUM()
enum class EPofPatchPhase : uint8
{
    /** No hot-patch in progress. */
    Idle,
    /** Writing file to disk. */
    WritingFile,
    /** Live Coding compile triggered and in progress. */
    Compiling,
    /** Compile succeeded, running verification logic. */
    Verifying,
    /** Verification passed — patch complete. */
    Complete,
    /** Compile or verification failed, reverting file. */
    Reverting,
    /** Patch failed and file was reverted. */
    Reverted,
    /** Compile failed but no revert was possible. */
    Failed
};

/**
 * A single diagnostic from the compiler output.
 */
USTRUCT()
struct FPofDiagnostic
{
    GENERATED_BODY()

    UPROPERTY()
    FString Severity;   // "error", "warning", "info"

    UPROPERTY()
    FString File;

    UPROPERTY()
    int32 Line = 0;

    UPROPERTY()
    FString Message;

    UPROPERTY()
    FString RawText;
};

/**
 * Result of a hot-patch operation: file write -> compile -> verify -> (revert on fail).
 */
USTRUCT()
struct FPofHotPatchResult
{
    GENERATED_BODY()

    UPROPERTY()
    EPofPatchPhase Phase = EPofPatchPhase::Idle;

    UPROPERTY()
    EPofCompileStatus CompileStatus = EPofCompileStatus::Idle;

    UPROPERTY()
    FString FilePath;

    UPROPERTY()
    double DurationMs = 0.0;

    UPROPERTY()
    bool bVerificationPassed = false;

    UPROPERTY()
    FString VerificationOutput;

    UPROPERTY()
    bool bFileReverted = false;

    UPROPERTY()
    FString ErrorMessage;

    UPROPERTY()
    TArray<FPofDiagnostic> Diagnostics;
};

UCLASS()
class UPofLiveCodingBridge : public UObject
{
    GENERATED_BODY()

public:
    /** Trigger a simple live coding compile and return result JSON. */
    FString TriggerCompile();

    /**
     * Full hot-patch workflow: write file -> compile -> verify -> revert on failure.
     * @param FilePath     Absolute path to the .cpp/.h file to write.
     * @param FileContent  The full file content to write.
     * @param VerifyObjectPath  Optional: UObject path for Remote Control verification call.
     * @param VerifyFunctionName Optional: UFUNCTION to call for verification after compile.
     * @return JSON string with FPofHotPatchResult data.
     */
    FString TriggerHotPatch(
        const FString& FilePath,
        const FString& FileContent,
        const FString& VerifyObjectPath,
        const FString& VerifyFunctionName
    );

    /** Get current compile status as a string. */
    FString GetCurrentStatus() const;

    /** Get current hot-patch phase as a string. */
    FString GetPatchPhase() const;

    /** Get full status JSON (compile status + patch phase + last result). */
    FString GetFullStatusJson() const;

private:
    void OnLiveCodingComplete();

    /** Back up a file before patching, returns true if backup was created. */
    bool BackupFile(const FString& FilePath);

    /** Restore a file from backup, returns true if restored. */
    bool RevertFile(const FString& FilePath);

    /** Run verification via Remote Control function call. */
    bool RunVerification(const FString& ObjectPath, const FString& FunctionName, FString& OutResult);

    /** Parse compiler output for diagnostics. */
    void ParseCompilerOutput(const FString& Output);

    /** Serialize the current HotPatchResult to JSON. */
    FString SerializeResult() const;

    EPofCompileStatus CurrentStatus = EPofCompileStatus::Idle;
    EPofPatchPhase PatchPhase = EPofPatchPhase::Idle;
    FString LastErrorMessage;
    FDateTime CompileStartTime;
    FPofHotPatchResult LastPatchResult;

    /** Original file content saved before hot-patching, for revert. */
    FString BackupContent;
    FString BackupFilePath;

    /** Whether the ILiveCodingModule delegate is bound. */
    bool bDelegateBound = false;

    /** Pending verification info (used after async compile completes). */
    FString PendingVerifyObjectPath;
    FString PendingVerifyFunctionName;
    FString PendingFilePath;
};

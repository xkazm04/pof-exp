#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PofTestRunner.generated.h"

UENUM()
enum class EPofTestStatus : uint8
{
    Pending,
    Running,
    Passed,
    Failed,
    Error,
    Timeout
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofSpawnEntry
{
    GENERATED_BODY()

    UPROPERTY() FString BlueprintPath;
    UPROPERTY() FString Tag;
    UPROPERTY() FVector Location = FVector::ZeroVector;
    UPROPERTY() FRotator Rotation = FRotator::ZeroRotator;
    UPROPERTY() TMap<FString, FString> PropertyOverrides;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofTestAction
{
    GENERATED_BODY()

    UPROPERTY() FString Type;
    UPROPERTY() FString Target;
    UPROPERTY() FString Function;
    UPROPERTY() float Duration = 0.0f;
    UPROPERTY() FString Reason;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofTestAssertion
{
    GENERATED_BODY()

    UPROPERTY() FString Id;
    UPROPERTY() FString Target;
    UPROPERTY() FString Property;
    UPROPERTY() FString Operator;
    UPROPERTY() FString Expected;
    UPROPERTY() FString Description;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofTestSpec
{
    GENERATED_BODY()

    UPROPERTY() FString TestId;
    UPROPERTY() FString Description;
    UPROPERTY() float Timeout = 30.0f;
    UPROPERTY() TArray<FPofSpawnEntry> Setup;
    UPROPERTY() TArray<FPofTestAction> Actions;
    UPROPERTY() TArray<FPofTestAssertion> Assertions;
    UPROPERTY() FString Cleanup;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofAssertionResult
{
    GENERATED_BODY()

    UPROPERTY() FString Id;
    UPROPERTY() FString Status;
    UPROPERTY() FString Description;
    UPROPERTY() FString Expected;
    UPROPERTY() FString Actual;
    UPROPERTY() FString FailureReason;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofTestLogEntry
{
    GENERATED_BODY()

    UPROPERTY() float Time = 0.0f;
    UPROPERTY() FString Message;
};

USTRUCT()
struct PILLARSOFFORTUNEBRIDGE_API FPofTestResult
{
    GENERATED_BODY()

    UPROPERTY() FString TestId;
    UPROPERTY() EPofTestStatus Status = EPofTestStatus::Pending;
    UPROPERTY() FString StartTime;
    UPROPERTY() FString EndTime;
    UPROPERTY() int32 DurationMs = 0;
    UPROPERTY() TArray<FPofAssertionResult> Assertions;
    UPROPERTY() TArray<FPofTestLogEntry> Logs;
    UPROPERTY() TArray<FString> Errors;
};

UCLASS()
class PILLARSOFFORTUNEBRIDGE_API UPofTestRunner : public UObject
{
    GENERATED_BODY()

public:
    void ExecuteTestSpec(const FPofTestSpec& Spec);
    const FPofTestResult& GetLastResult() const { return LastResult; }
    TArray<FPofTestResult> GetAllResults() const { return StoredResults; }

    bool IsRunning() const { return bIsRunning; }

    DECLARE_DELEGATE_OneParam(FOnTestComplete, const FPofTestResult&);
    FOnTestComplete OnTestComplete;

private:
    void ContinueExecution();
    void ExecuteNextAction();
    void RunAssertions();
    void FailTest(const FString& Reason);
    void CompleteTest(EPofTestStatus Status);
    void OnPIEStarted(bool bIsSimulating);
    UWorld* GetPIEWorld() const;

    FPofTestSpec CurrentSpec;
    FPofTestResult LastResult;
    TArray<FPofTestResult> StoredResults;
    TMap<FString, AActor*> SpawnedActors;
    int32 CurrentActionIndex = 0;
    bool bIsRunning = false;
    FTimerHandle ActionTimerHandle;
    FTimerHandle TimeoutHandle;
    FDateTime TestStartTime;
};

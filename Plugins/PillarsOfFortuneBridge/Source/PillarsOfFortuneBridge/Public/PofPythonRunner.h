// Copyright PoF. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PofPythonRunner.generated.h"

/**
 * Outcome of a Python invocation through the bridge.
 * `bOk` mirrors the structured result the Python wrapper marks; on transport
 * failures (no plugin, bad JSON), `bOk=false` and `ErrorMessage` explains.
 */
USTRUCT()
struct FPofPythonOutcome
{
    GENERATED_BODY()

    UPROPERTY()
    bool bOk = false;

    /** Raw JSON the Python wrapper printed (parsed from the __POF_BRIDGE_RESULT__ marker line). */
    UPROPERTY()
    FString ResultJson;

    /** Transport-level error if the call never produced a marker. Empty when ResultJson is set. */
    UPROPERTY()
    FString ErrorMessage;
};

/**
 * Bridge invocation point for `import <module>; <function>(args)`.
 *
 * Keeps Python execution on the editor thread (Python plugin requirement) and
 * returns structured JSON the HTTP layer forwards verbatim. Captures stdout
 * and stderr around the call and surfaces a traceback on exception.
 */
UCLASS()
class PILLARSOFFORTUNEBRIDGE_API UPofPythonRunner : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Dispatch `<Module>.<Function>(json.loads(ArgsJson))` and return the structured
     * outcome. The Python wrapper always prints a `__POF_BRIDGE_RESULT__<json>` marker
     * line so transport-level failures can be distinguished from in-Python errors.
     */
    FPofPythonOutcome Run(const FString& Module, const FString& Function, const FString& ArgsJson);
};

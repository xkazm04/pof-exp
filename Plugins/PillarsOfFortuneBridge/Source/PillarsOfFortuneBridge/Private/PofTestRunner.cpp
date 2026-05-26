#include "PofTestRunner.h"
#include "PillarsOfFortuneBridge.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

#if WITH_EDITOR
#include "Editor.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#endif

#if WITH_DEV_AUTOMATION_TESTS || WITH_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif

FPofAutomationOutcome UPofTestRunner::RunAutomationTest(const FString& Filter)
{
    FPofAutomationOutcome Outcome;
#if WITH_DEV_AUTOMATION_TESTS || WITH_AUTOMATION_TESTS
    FAutomationTestFramework& Framework = FAutomationTestFramework::Get();

    // GetValidTestNames is gated by the RequestedTestFilter (0 in a fresh editor that
    // never opened the Automation UI → empty list). Request all filter bits so
    // EngineFilter/ProductFilter/etc. tests are enumerable.
    Framework.SetRequestedTestFilter(EAutomationTestFlags_FilterMask);

    // Find a registered test whose full or display name contains the filter.
    TArray<FAutomationTestInfo> TestInfos;
    Framework.GetValidTestNames(TestInfos);
    FString MatchedName;
    for (const FAutomationTestInfo& Info : TestInfos)
    {
        if (Info.GetTestName().Contains(Filter) || Info.GetDisplayName().Contains(Filter))
        {
            MatchedName = Info.GetTestName();
            break;
        }
    }
    if (MatchedName.IsEmpty())
    {
        Outcome.Message = FString::Printf(TEXT("No automation test matches '%s'"), *Filter);
        UE_LOG(LogPofBridge, Warning, TEXT("RunAutomationTest: %s"), *Outcome.Message);
        return Outcome;
    }

    // Simple automation tests run synchronously within StartTestByName/StopTest.
    const FDateTime StartedAt = FDateTime::UtcNow();
    Framework.StartTestByName(MatchedName, 0);
    FAutomationTestExecutionInfo ExecInfo;
    const bool bPassed = Framework.StopTest(ExecInfo);
    const FDateTime EndedAt = FDateTime::UtcNow();

    Outcome.bFound = true;
    Outcome.bPassed = bPassed;
    Outcome.MatchedName = MatchedName;

    // Store a result so GET /pof/test/results returns it (keyed by the matched name).
    FPofTestResult Result;
    Result.TestId = MatchedName;
    Result.Status = bPassed ? EPofTestStatus::Passed : EPofTestStatus::Failed;
    Result.StartTime = StartedAt.ToIso8601();
    Result.EndTime = EndedAt.ToIso8601();
    Result.DurationMs = static_cast<int32>((EndedAt - StartedAt).GetTotalMilliseconds());
    for (const FAutomationExecutionEntry& Entry : ExecInfo.GetEntries())
    {
        if (Entry.Event.Type == EAutomationEventType::Error)
        {
            Result.Errors.Add(Entry.Event.Message);
        }
    }
    StoredResults.Add(Result);
    UE_LOG(LogPofBridge, Log, TEXT("RunAutomationTest '%s' -> %s (%dms)"), *MatchedName,
        bPassed ? TEXT("passed") : TEXT("failed"), Result.DurationMs);
#else
    Outcome.Message = TEXT("Automation framework unavailable in this build configuration");
#endif
    return Outcome;
}

void UPofTestRunner::ExecuteTestSpec(const FPofTestSpec& Spec)
{
	if (bIsRunning)
	{
		UE_LOG(LogPofBridge, Warning, TEXT("Test already running, ignoring new spec: %s"), *Spec.TestId);
		return;
	}

	CurrentSpec = Spec;
	LastResult = FPofTestResult();
	LastResult.TestId = Spec.TestId;
	TestStartTime = FDateTime::UtcNow();
	LastResult.StartTime = TestStartTime.ToIso8601();
	LastResult.Status = EPofTestStatus::Running;
	bIsRunning = true;
	CurrentActionIndex = 0;
	SpawnedActors.Empty();

	UE_LOG(LogPofBridge, Log, TEXT("Starting test: %s"), *Spec.TestId);

#if WITH_EDITOR
	if (GEditor && !GEditor->IsPlaySessionInProgress())
	{
		FRequestPlaySessionParams Params;
		GEditor->RequestPlaySession(Params);

		// Wait for PIE to start, then continue
		FEditorDelegates::PostPIEStarted.AddUObject(this, &UPofTestRunner::OnPIEStarted);
		return;
	}
#endif

	ContinueExecution();
}

void UPofTestRunner::OnPIEStarted(bool bIsSimulating)
{
#if WITH_EDITOR
	FEditorDelegates::PostPIEStarted.RemoveAll(this);
#endif
	ContinueExecution();
}

void UPofTestRunner::ContinueExecution()
{
	UWorld* PIEWorld = GetPIEWorld();
	if (!PIEWorld)
	{
		FailTest(TEXT("No PIE world available"));
		return;
	}

	for (const FPofSpawnEntry& SpawnEntry : CurrentSpec.Setup)
	{
		UClass* ActorClass = LoadClass<AActor>(nullptr, *SpawnEntry.BlueprintPath);
		if (!ActorClass)
		{
			FailTest(FString::Printf(TEXT("Failed to load class: %s"), *SpawnEntry.BlueprintPath));
			return;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Name = FName(*SpawnEntry.Tag);
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AActor* Actor = PIEWorld->SpawnActor<AActor>(ActorClass, SpawnEntry.Location, SpawnEntry.Rotation, SpawnParams);

		if (!Actor)
		{
			FailTest(FString::Printf(TEXT("Failed to spawn: %s"), *SpawnEntry.Tag));
			return;
		}

		for (const auto& Override : SpawnEntry.PropertyOverrides)
		{
			FProperty* Prop = Actor->GetClass()->FindPropertyByName(FName(*Override.Key));
			if (Prop)
			{
				void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Actor);
				Prop->ImportText_Direct(*Override.Value, ValuePtr, Actor, PPF_None);
			}
		}

		SpawnedActors.Add(SpawnEntry.Tag, Actor);

		FPofTestLogEntry Log;
		Log.Time = (FDateTime::UtcNow() - TestStartTime).GetTotalSeconds();
		Log.Message = FString::Printf(TEXT("Spawned %s at (%s)"), *SpawnEntry.Tag, *SpawnEntry.Location.ToString());
		LastResult.Logs.Add(Log);
	}

	ExecuteNextAction();
}

void UPofTestRunner::ExecuteNextAction()
{
	if (CurrentActionIndex >= CurrentSpec.Actions.Num())
	{
		RunAssertions();
		return;
	}

	const FPofTestAction& Action = CurrentSpec.Actions[CurrentActionIndex];
	CurrentActionIndex++;

	if (Action.Type == TEXT("call"))
	{
		AActor* Target = SpawnedActors.FindRef(Action.Target);
		if (!Target)
		{
			FailTest(FString::Printf(TEXT("Target not found: %s"), *Action.Target));
			return;
		}

		UFunction* Func = Target->FindFunction(FName(*Action.Function));
		if (!Func)
		{
			FailTest(FString::Printf(TEXT("Function not found: %s.%s"), *Action.Target, *Action.Function));
			return;
		}

		Target->ProcessEvent(Func, nullptr);

		FPofTestLogEntry Log;
		Log.Time = (FDateTime::UtcNow() - TestStartTime).GetTotalSeconds();
		Log.Message = FString::Printf(TEXT("Called %s.%s"), *Action.Target, *Action.Function);
		LastResult.Logs.Add(Log);

		ExecuteNextAction();
	}
	else if (Action.Type == TEXT("wait"))
	{
		UWorld* World = GetPIEWorld();
		if (World)
		{
			World->GetTimerManager().SetTimer(
				ActionTimerHandle, this, &UPofTestRunner::ExecuteNextAction, Action.Duration, false);
		}
	}
}

void UPofTestRunner::RunAssertions()
{
	for (const FPofTestAssertion& Assert : CurrentSpec.Assertions)
	{
		FPofAssertionResult Result;
		Result.Id = Assert.Id;
		Result.Description = Assert.Description;

		AActor* Target = SpawnedActors.FindRef(Assert.Target);
		if (!Target)
		{
			Result.Status = TEXT("error");
			Result.FailureReason = FString::Printf(TEXT("Target actor '%s' not found"), *Assert.Target);
			LastResult.Assertions.Add(Result);
			continue;
		}

		FProperty* Prop = Target->GetClass()->FindPropertyByName(FName(*Assert.Property));
		if (!Prop)
		{
			Result.Status = TEXT("error");
			Result.FailureReason = FString::Printf(TEXT("Property '%s' not found on %s"), *Assert.Property, *Assert.Target);
			LastResult.Assertions.Add(Result);
			continue;
		}

		FString ActualStr;
		void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Target);
		Prop->ExportTextItem_Direct(ActualStr, ValuePtr, nullptr, Target, PPF_None);
		Result.Actual = ActualStr;
		Result.Expected = Assert.Operator + TEXT(" ") + Assert.Expected;

		float ActualVal = FCString::Atof(*ActualStr);
		float ExpectedVal = FCString::Atof(*Assert.Expected);

		bool bPassed = false;
		if (Assert.Operator == TEXT("equals")) bPassed = FMath::IsNearlyEqual(ActualVal, ExpectedVal);
		else if (Assert.Operator == TEXT("notEquals")) bPassed = !FMath::IsNearlyEqual(ActualVal, ExpectedVal);
		else if (Assert.Operator == TEXT("greaterThan")) bPassed = ActualVal > ExpectedVal;
		else if (Assert.Operator == TEXT("lessThan")) bPassed = ActualVal < ExpectedVal;
		else if (Assert.Operator == TEXT("greaterThanOrEqual")) bPassed = ActualVal >= ExpectedVal;
		else if (Assert.Operator == TEXT("lessThanOrEqual")) bPassed = ActualVal <= ExpectedVal;
		else if (Assert.Operator == TEXT("isTrue")) bPassed = ActualStr.ToBool();
		else if (Assert.Operator == TEXT("isFalse")) bPassed = !ActualStr.ToBool();

		Result.Status = bPassed ? TEXT("passed") : TEXT("failed");
		if (!bPassed)
		{
			Result.FailureReason = FString::Printf(TEXT("'%s' %s %s was false (actual: %s)"),
				*Assert.Property, *Assert.Operator, *Assert.Expected, *ActualStr);
		}

		LastResult.Assertions.Add(Result);
	}

	bool bAllPassed = true;
	for (const FPofAssertionResult& R : LastResult.Assertions)
	{
		if (R.Status != TEXT("passed"))
		{
			bAllPassed = false;
			break;
		}
	}

	CompleteTest(bAllPassed ? EPofTestStatus::Passed : EPofTestStatus::Failed);
}

void UPofTestRunner::FailTest(const FString& Reason)
{
	LastResult.Errors.Add(Reason);
	CompleteTest(EPofTestStatus::Error);
}

void UPofTestRunner::CompleteTest(EPofTestStatus Status)
{
	LastResult.Status = Status;
	LastResult.EndTime = FDateTime::UtcNow().ToIso8601();
	LastResult.DurationMs = (int32)(FDateTime::UtcNow() - TestStartTime).GetTotalMilliseconds();
	bIsRunning = false;

	StoredResults.Add(LastResult);

	UE_LOG(LogPofBridge, Log, TEXT("Test %s completed: %s"),
		*LastResult.TestId,
		Status == EPofTestStatus::Passed ? TEXT("PASSED") : TEXT("FAILED"));

	if (OnTestComplete.IsBound())
	{
		OnTestComplete.Execute(LastResult);
	}
}

UWorld* UPofTestRunner::GetPIEWorld() const
{
#if WITH_EDITOR
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE && Context.World())
			{
				return Context.World();
			}
		}
	}
#endif
	return nullptr;
}

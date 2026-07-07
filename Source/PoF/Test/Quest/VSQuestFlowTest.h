#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "VSQuestFlowTest.generated.h"

/**
 * Minimal listener for the quest automation gate. UARPGQuestSubsystem's
 * lifecycle events (OnQuestAccepted / OnQuestCompleted / OnQuestFailed /
 * OnObjectiveUpdated) are dynamic multicast delegates (Blueprint-assignable
 * so quest-log UI can bind in BP), so counting their broadcasts from the
 * simple-automation test requires a UFUNCTION target on a UObject.
 *
 * See VSQuestFlowTest.cpp — the actual gate is an
 * IMPLEMENT_SIMPLE_AUTOMATION_TEST (no map / no PIE), mirroring the
 * currency wallet gate (Test/Economy/VSCurrencyWalletTest.cpp).
 */
UCLASS()
class UVSQuestEventCounter : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 AcceptedCount = 0;

	UPROPERTY()
	int32 CompletedCount = 0;

	UPROPERTY()
	int32 FailedCount = 0;

	UPROPERTY()
	int32 ObjectiveUpdateCount = 0;

	UPROPERTY()
	FName LastCompletedQuestID;

	UFUNCTION()
	void OnAccepted(FName QuestID)
	{
		++AcceptedCount;
	}

	UFUNCTION()
	void OnCompleted(FName QuestID)
	{
		++CompletedCount;
		LastCompletedQuestID = QuestID;
	}

	UFUNCTION()
	void OnFailed(FName QuestID)
	{
		++FailedCount;
	}

	UFUNCTION()
	void OnObjectiveUpdated(FName QuestID, int32 ObjectiveIndex, int32 NewCount)
	{
		++ObjectiveUpdateCount;
	}
};

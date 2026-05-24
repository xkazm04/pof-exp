#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSArenaSetupTest.generated.h"

/** Asserts the arena's lighting + post-process actors are present (headless setup invariant). */
UCLASS()
class POF_API AVSArenaSetupTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AVSArenaSetupTest();

	virtual void StartTest() override;
};

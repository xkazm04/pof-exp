#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSFootstepWiringTest.generated.h"

/** Asserts the footstep-stone audio set imported correctly into /Game/Audio/footstep-stone/. */
UCLASS()
class POF_API AVSFootstepWiringTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AVSFootstepWiringTest();

	virtual void StartTest() override;
};

#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSArenaCollisionTest.generated.h"

/** Drops physics probes onto the arena floor; asserts they rest (collision holds). */
UCLASS()
class POF_API AVSArenaCollisionTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AVSArenaCollisionTest();

	virtual void StartTest() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	float Elapsed = 0.f;
	bool bAsserted = false;
	TArray<TWeakObjectPtr<AActor>> Probes;
};

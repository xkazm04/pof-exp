#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSArenaMaterialBindingTest.generated.h"

/**
 * Material-binding invariant for SM_Arena (folder-06 tests.md UE #1).
 *
 * Asset-inspection test (no world mutation — safe sibling under the shared PIE
 * world; deterministic under -nullrhi since it reads asset objects, not pixels):
 *  - every material slot on SM_Arena is non-null;
 *  - for any slot bound to a MaterialInstance of the surface master, its Albedo
 *    texture parameter resolves and its Normal/Roughness textures are sampled
 *    non-sRGB (the PS-2 regression class: a normal sampled as sRGB renders washed
 *    out). Plain (non-instance) slots get the non-null check only.
 */
UCLASS()
class POF_API AVSArenaMaterialBindingTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AVSArenaMaterialBindingTest();

	virtual void StartTest() override;
};

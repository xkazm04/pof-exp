#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSMasterMaterialInstanceTest.generated.h"

/**
 * Master-material consolidation invariant (folder-06 tests.md UE #3 / game §2).
 *
 * Asserts the surface master exists and that each consolidation instance
 * (MI_Arena_Floor/Wall/Pillar + MI_EnemyRed) is a MaterialInstance parented to
 * /Game/Materials/M_ARPG_Surface_Master. Detects a "someone re-created a one-off
 * material" regression. Inspects the /Game/Materials/ assets directly, so it is
 * independent of SM_Arena's current slot assignment (which a parallel
 * environment branch may have reverted to plain materials).
 */
UCLASS()
class POF_API AVSMasterMaterialInstanceTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AVSMasterMaterialInstanceTest();

	virtual void StartTest() override;
};

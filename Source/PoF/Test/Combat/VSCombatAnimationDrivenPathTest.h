#pragma once

#include "CoreMinimal.h"
#include "FunctionalTest.h"
#include "VSCombatAnimationDrivenPathTest.generated.h"

// DISABLED until a real AM_MeleeCombo montage with AnimNotifyState_HitDetection exists.
// When it does: set bUseAnimationDrivenDamage=true on BP_GA_MeleeAttack, play the
// montage, advance it past the hit-detection window, and assert damage applied via
// OnMeleeHit (e.g. a counter or the enemy Health drop) WITHOUT the test sending
// Event.MeleeHit. The companion AVSCombatGrayBoxPathTest covers the false path.

/**
 * Animation-driven melee damage path (bUseAnimationDrivenDamage=true). Gated on a
 * real attack montage carrying the hit-detection notify; registered DISABLED so it
 * documents the intended coverage without failing the gray-box slice.
 */
UCLASS()
class POF_API AVSCombatAnimationDrivenPathTest : public AFunctionalTest
{
	GENERATED_BODY()

public:
	AVSCombatAnimationDrivenPathTest();

	virtual void StartTest() override;
};

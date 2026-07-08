#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGTutorialRules.generated.h"

/**
 * Tutorial beat gating rules (catalog pipeline: tutorial-beats). A beat fires on
 * zone entry only while its "Introduced" tag is absent (idempotent — never re-fires
 * once set), and offensive inputs are locked while the sandbox beat is active.
 * Mirrors src/lib/catalog/pipelines/tutorial-beats.ts.
 */
UCLASS()
class POF_API UARPGTutorialRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Fire the beat on OnEnterZone only when its Introduced tag is not yet set. */
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	static bool ShouldTriggerBeat(bool bIntroducedTagPresent) { return !bIntroducedTagPresent; }

	/** Offensive inputs (IA_Attack / IA_Skill_*) are locked while the sandbox is active. */
	UFUNCTION(BlueprintPure, Category = "Tutorial")
	static bool AreOffensiveInputsLocked(bool bSandboxActive) { return bSandboxActive; }
};

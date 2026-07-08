#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGCodexRules.generated.h"

/**
 * Codex entry unlock rules (catalog pipeline: codex). The "codex-sundering" entry
 * unlocks via a primary path (quest-ember-pact reaches stage 1) OR a fallback path
 * (Ashen Forest zone entry) — and NOT before either. Mirrors
 * src/lib/catalog/pipelines/codex.ts.
 */
UCLASS()
class POF_API UARPGCodexRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr int32 PrimaryUnlockQuestStage = 1;

	/** True once EITHER the quest reaches stage 1 (primary) OR the zone was entered
	 *  (fallback). False before either trigger. */
	UFUNCTION(BlueprintPure, Category = "Codex")
	static bool ShouldUnlockEntry(int32 QuestStage, bool bZoneEntered)
	{ return QuestStage >= PrimaryUnlockQuestStage || bZoneEntered; }
};

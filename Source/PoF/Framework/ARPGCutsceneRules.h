#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGCutsceneRules.generated.h"

/**
 * Prologue cutscene timing rules (catalog pipeline: cutscenes). ~90 s LevelSequence,
 * skippable after a 3 s grace window, with ordered beat markers at fixed timecodes.
 * Mirrors src/lib/catalog/pipelines/cutscenes.ts (beats + design intent).
 */
UCLASS()
class POF_API UARPGCutsceneRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static constexpr float TotalDurationSeconds = 90.f;
	static constexpr float SkipGraceSeconds = 3.f;

	// Beat marker timecodes (seconds) — must stay ordered & inside [0, TotalDuration].
	static constexpr float TcVaelIntroEnter = 38.f;
	static constexpr float TcVaelEyeContact = 58.f;
	static constexpr float TcEmberCrescendo = 72.f;
	static constexpr float TcEnd = 82.f;

	/** Skip is allowed only after the grace window. */
	UFUNCTION(BlueprintPure, Category = "Cutscene")
	static bool IsSkippable(float TimeCodeSeconds) { return TimeCodeSeconds >= SkipGraceSeconds; }

	/** True when the four beat markers are strictly increasing and within the sequence. */
	UFUNCTION(BlueprintPure, Category = "Cutscene")
	static bool BeatsAreValid()
	{
		return TcVaelIntroEnter < TcVaelEyeContact
			&& TcVaelEyeContact < TcEmberCrescendo
			&& TcEmberCrescendo < TcEnd
			&& TcEnd < TotalDurationSeconds
			&& TcVaelIntroEnter > 0.f;
	}
};

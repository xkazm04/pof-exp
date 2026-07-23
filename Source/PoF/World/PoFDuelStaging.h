#pragma once

#include "CoreMinimal.h"

class APawn;
class UWorld;

/**
 * Duel staging — applies the pre-game menu selections to the world:
 *   • saber choice → recolor blade-ish components on the player pawn
 *     (mesh emissive MIDs + lights whose names contain saber/blade/beam)
 *   • quest choice → arena staging (unbound post-process tint + fog),
 *     palettes mirrored code-as-data from the quests SOR
 *     (Triggers & World-State `environment` — single source; see the
 *     browser preview's applyArenaTheme for the sibling realization).
 * Everything logs what it touched — runtime evidence over assumptions.
 */
namespace PoFDuelStaging
{
	struct FSaberPalette
	{
		FName Id;
		FLinearColor Color;
	};

	struct FQuestPalette
	{
		FName Id;              // e.g. "lords-challenge"
		FString DisplayName;
		FLinearColor FogColor;     // SOR environment.fogColor
		FLinearColor LightTint;    // SOR environment.lightTint
		FLinearColor FloorEmissive;// SOR environment.floorEmissive
	};

	POF_API TArray<FSaberPalette> GetSaberPalettes();
	POF_API TArray<FQuestPalette> GetQuestPalettes();
	POF_API const FSaberPalette* FindSaber(FName Id);
	POF_API const FQuestPalette* FindQuest(FName Id);

	/** Recolor blade-ish components on the pawn. Returns number of components touched. */
	POF_API int32 ApplySaberChoice(APawn* Pawn, FName SaberId);

	/** Apply the quest's arena staging (post-process tint + fog) to the world. */
	POF_API bool ApplyQuestStaging(UWorld* World, FName QuestId);
}

#pragma once

#include "CoreMinimal.h"

class UARPGDialogueTree;

/**
 * Duel Challenge — the pre-combat exchange with Darth Malgrave (dialog-trees
 * catalog: dialog-duel-intro). Code-as-data single source mirroring the SOR
 * Branch Graph, same pattern as AARPGEnemyCharacter::GetArchetypeDefaults:
 * the browser preview and this builder realize the SAME authored tree.
 *
 * SOR node → UE encoding: player-line nodes fold into FARPGDialogueChoice
 * entries (the UE model's native choice shape); the END sentinel becomes
 * NextNodeIndex -1. Two choices (defy / silent ignite), both reach combat.
 */
namespace ARPGDuelIntroDialogue
{
	/** Fill an (empty or transient) tree with the duel-intro nodes. */
	POF_API void Populate(UARPGDialogueTree& Tree);
}

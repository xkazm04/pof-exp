#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "ARPGCheatManager.generated.h"

/**
 * ARPG Cheat Manager — console commands for rapid testing.
 *
 * Registered automatically when set as CheatClass on APlayerController.
 * All commands are prefixed with "ARPG." in the console.
 *
 * Commands:
 *   ARPG.God              — Toggle god mode (invulnerable + infinite mana)
 *   ARPG.SetLevel <N>     — Set player to level N
 *   ARPG.GiveXP <Amount>  — Award XP
 *   ARPG.Heal             — Fully restore health and mana
 *   ARPG.KillAll          — Kill all enemies in the level
 *   ARPG.SpawnEnemy <Class> [Level] — Spawn enemy in front of player
 *   ARPG.DamageTarget <Amount>      — Deal damage to crosshair target
 *   ARPG.ShowStats        — Print player stats to screen
 */
UCLASS()
class POF_API UARPGCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	// =====================================================================
	// God Mode
	// =====================================================================

	/** Toggle god mode: invulnerable + infinite mana. */
	UFUNCTION(Exec)
	void ARPGGod();

	// =====================================================================
	// Progression
	// =====================================================================

	/** Set the player's character level directly. */
	UFUNCTION(Exec)
	void ARPGSetLevel(int32 NewLevel);

	/** Award XP to the player. */
	UFUNCTION(Exec)
	void ARPGGiveXP(float Amount);

	// =====================================================================
	// Vitals
	// =====================================================================

	/** Fully restore health and mana. */
	UFUNCTION(Exec)
	void ARPGHeal();

	// =====================================================================
	// Combat
	// =====================================================================

	/** Kill all enemy characters in the world. */
	UFUNCTION(Exec)
	void ARPGKillAll();

	/** Spawn an enemy at the player's look location. */
	UFUNCTION(Exec)
	void ARPGSpawnEnemy(const FString& BlueprintPath, int32 EnemyLevel = 1);

	/** Deal raw damage to the crosshair target. */
	UFUNCTION(Exec)
	void ARPGDamageTarget(float Amount);

	// =====================================================================
	// Diagnostics
	// =====================================================================

	/** Print player attribute stats to screen. */
	UFUNCTION(Exec)
	void ARPGShowStats();

private:
	bool bGodMode = false;
};

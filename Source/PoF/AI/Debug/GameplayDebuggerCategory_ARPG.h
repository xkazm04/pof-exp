#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "GameplayDebuggerCategory.h"

class AActor;
class APlayerController;

/**
 * Custom gameplay debugger category for ARPG AI.
 * Displays:
 *  - Current BT state (active task/service)
 *  - Blackboard key values (target, distances, archetype)
 *  - Perception state (LOS, detected actors)
 *  - Squad info (attack tokens, flank positions)
 *  - GAS state tags
 *  - Combat stats (health, attack range, cooldown)
 *
 * Activated via Gameplay Debugger (default: ' key) under the "ARPG AI" category.
 */
class FGameplayDebuggerCategory_ARPG : public FGameplayDebuggerCategory
{
public:
	FGameplayDebuggerCategory_ARPG();

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

protected:
	struct FRepData
	{
		FString ArchetypeName;
		FString ActiveBTTask;
		FString TargetName;
		float DistanceToTarget = 0.f;
		bool bHasLOS = false;
		float HealthPercent = 0.f;
		float AttackRange = 0.f;
		float AttackCooldown = 0.f;
		FString ActiveGASTags;
		bool bHasAttackToken = false;
		int32 SquadMemberCount = 0;
		FString AIState; // "Idle", "Combat", "Retreating", etc.

		void Serialize(FArchive& Ar);
	};

	FRepData DataPack;
};

#endif // WITH_GAMEPLAY_DEBUGGER

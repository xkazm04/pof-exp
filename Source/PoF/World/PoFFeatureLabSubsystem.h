#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "PoFFeatureLabSubsystem.generated.h"

/**
 * FeatureLab — the clean default map's runtime populator.
 *
 * /Game/Maps/FeatureLab stays an EMPTY floor; every feature under test is
 * spawned HERE at world begin-play from the code-as-data roster (GetRoster).
 * Adding a feature to the lab = adding a roster entry — no level edits, no
 * editor scripting, reviewable in a diff, asserted by the FeatureLab gate.
 *
 * Current roster: Darth Malgrave (Duel Challenge speaker — walk up, press F).
 */

USTRUCT()
struct FFeatureLabEntry
{
	GENERATED_BODY()

	/** Class path (LoadClass at spawn time so the roster stays data). */
	FString ClassPath;
	/** Actor label in the world outliner / logs. */
	FString Label;
	/** Spawn offset from the PlayerStart (falls back to world origin). */
	FVector Offset = FVector::ZeroVector;
	/** Optional NPCID applied when the actor is an AARPGNPCActor. */
	FName NPCID;
};

UCLASS()
class POF_API UPoFFeatureLabSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Pure: does this map name belong to the FeatureLab? (PIE prefixes tolerated.) */
	static bool ShouldPopulate(const FString& MapName);

	/** Pure code-as-data roster — the single source the gate asserts. */
	static TArray<FFeatureLabEntry> GetRoster();
};

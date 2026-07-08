#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ARPGMusicRules.generated.h"

/**
 * Dynamic-music transition rules (catalog pipeline: music). Maps gameplay MusicEvents
 * to the active music layer and defines the boss-swell HP trigger. Mirrors
 * src/lib/catalog/pipelines/music.ts. Companion to the runtime UARPGMusicManager
 * (EMusicState) — this is the pure event→layer decision the manager applies.
 */
UENUM(BlueprintType)
enum class EARPGMusicLayer : uint8
{
	Silence,
	AmbientTension,
	CombatLow,
	CombatHigh,
	BossSwell
};

UCLASS()
class POF_API UARPGMusicRules : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Elite HP fraction at or below which the boss-swell layer triggers. */
	static constexpr float BossSwellHpThreshold = 0.30f;

	/** The layer a MusicEvent selects: AmbientStart→AmbientTension, CombatStart→CombatLow,
	 *  EliteSpawned→CombatHigh, BossSwell→BossSwell; unknown→Silence. */
	UFUNCTION(BlueprintPure, Category = "Audio|Music")
	static EARPGMusicLayer LayerForEvent(FName MusicEvent);

	/** Boss-swell triggers when the elite drops to ≤30% HP. */
	UFUNCTION(BlueprintPure, Category = "Audio|Music")
	static bool ShouldBossSwell(float EliteHpPct) { return EliteHpPct <= BossSwellHpThreshold; }
};

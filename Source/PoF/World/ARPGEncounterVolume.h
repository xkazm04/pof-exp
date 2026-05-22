#pragma once

#include "CoreMinimal.h"
#include "Spawning/SpawnVolume.h"
#include "ARPGEncounterVolume.generated.h"

class UARPGZoneDefinition;

/** Difficulty arc position within a zone — determines encounter scaling. */
UENUM(BlueprintType)
enum class EEncounterPosition : uint8
{
	/** Near zone entrance — lighter encounters. */
	Entrance   UMETA(DisplayName = "Entrance"),
	/** Middle of zone — standard difficulty. */
	Middle     UMETA(DisplayName = "Middle"),
	/** Deep in zone / near boss — harder encounters. */
	Deep       UMETA(DisplayName = "Deep"),
	/** Boss encounter. */
	Boss       UMETA(DisplayName = "Boss")
};

/**
 * An encounter volume tied to a zone definition.
 * Extends SpawnVolume with zone-aware difficulty scaling:
 * the encounter's difficulty is derived from the zone definition
 * and its position within the zone (entrance→deep difficulty arc).
 *
 * Configure the Waves array on the parent SpawnVolume,
 * or leave it empty and set bUseZoneEncounters to auto-populate
 * waves from the zone definition's encounter groups.
 */
UCLASS()
class POF_API AARPGEncounterVolume : public ASpawnVolume
{
	GENERATED_BODY()

public:
	AARPGEncounterVolume();

	UFUNCTION(BlueprintPure, Category = "World|Encounters")
	EEncounterPosition GetEncounterPosition() const { return EncounterPosition; }

	UFUNCTION(BlueprintPure, Category = "World|Encounters")
	UARPGZoneDefinition* GetZoneDefinition() const { return ZoneDefinition; }

protected:
	virtual void BeginPlay() override;

	/** The zone this encounter belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Zone")
	TObjectPtr<UARPGZoneDefinition> ZoneDefinition;

	/** Where in the zone this encounter sits (affects difficulty arc). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Zone")
	EEncounterPosition EncounterPosition = EEncounterPosition::Middle;

	/**
	 * If true and Waves is empty, auto-generate waves from the zone's
	 * encounter groups on BeginPlay.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Zone")
	bool bUseZoneEncounters = false;

	/** Number of waves to auto-generate when using zone encounters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter|Zone",
		meta = (EditCondition = "bUseZoneEncounters", ClampMin = "1"))
	int32 AutoWaveCount = 2;

private:
	void ApplyZoneDifficulty();
	void GenerateWavesFromZone();
	float GetPositionDifficultyMultiplier() const;
};

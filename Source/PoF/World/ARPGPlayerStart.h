#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "ARPGPlayerStart.generated.h"

class UARPGZoneDefinition;

/**
 * Extended player start that belongs to a zone.
 * Used as spawn/teleport targets for zone transitions and fast travel.
 * Tag the actor with a name matching ZoneTransition::TargetSpawnPointTag.
 */
UCLASS()
class POF_API AARPGPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	/** The zone this spawn point belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	TObjectPtr<UARPGZoneDefinition> OwningZone;

	/** If true, this is the default spawn point for the zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	bool bIsZoneDefault = false;
};

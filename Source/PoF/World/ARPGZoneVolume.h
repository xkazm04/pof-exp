#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGZoneVolume.generated.h"

class UARPGZoneDefinition;
class UBoxComponent;
class UBillboardComponent;

/**
 * A volume placed in the level that defines the spatial bounds of a zone.
 * When the player overlaps this volume, the ZoneManager is notified
 * and the player's current zone is updated.
 *
 * Also registers the zone definition with the ZoneManager on BeginPlay.
 */
UCLASS()
class POF_API AARPGZoneVolume : public AActor
{
	GENERATED_BODY()

public:
	AARPGZoneVolume();

	UFUNCTION(BlueprintPure, Category = "World|Zones")
	UARPGZoneDefinition* GetZoneDefinition() const { return ZoneDefinition; }

protected:
	virtual void BeginPlay() override;

	/** The zone this volume belongs to. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	TObjectPtr<UARPGZoneDefinition> ZoneDefinition;

	/** If true, this is the starting zone — player is placed here on load. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
	bool bIsStartingZone = false;

	/** Trigger bounds. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zone")
	TObjectPtr<UBoxComponent> ZoneBounds;

private:
	UFUNCTION()
	void OnPlayerEnterZone(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "Zone")
	TObjectPtr<UBillboardComponent> EditorSprite;
#endif
};

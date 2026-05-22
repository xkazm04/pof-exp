#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGNavModifierVolume.generated.h"

class UBoxComponent;
class UNavModifierComponent;

/** Types of navigation modification for zone design. */
UENUM(BlueprintType)
enum class ENavModifierType : uint8
{
	/** AI avoids this area unless pursuing a target. */
	Avoidance   UMETA(DisplayName = "Avoidance"),
	/** AI prefers this path (patrol routes, corridors). */
	Preferred   UMETA(DisplayName = "Preferred Path"),
	/** Completely blocks AI navigation. */
	Blocked     UMETA(DisplayName = "Blocked"),
	/** Narrow passage — AI enters single-file. */
	Chokepoint  UMETA(DisplayName = "Chokepoint")
};

/**
 * A navigation modifier volume for zone design.
 * Place these to shape AI pathfinding behavior within zones:
 * - Mark chokepoints and corridors
 * - Block areas from AI navigation
 * - Create preferred patrol routes
 * - Define avoidance zones
 *
 * Uses UNavModifierComponent under the hood to affect NavMesh costs.
 */
UCLASS()
class POF_API AARPGNavModifierVolume : public AActor
{
	GENERATED_BODY()

public:
	AARPGNavModifierVolume();

	UFUNCTION(BlueprintPure, Category = "Navigation")
	ENavModifierType GetModifierType() const { return ModifierType; }

protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** What kind of navigation modification this applies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation")
	ENavModifierType ModifierType = ENavModifierType::Avoidance;

	/** The area bounds. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	TObjectPtr<UBoxComponent> VolumeBounds;

	/** The nav modifier component that affects NavMesh generation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	TObjectPtr<UNavModifierComponent> NavModifier;

private:
	void ApplyModifierSettings();
};

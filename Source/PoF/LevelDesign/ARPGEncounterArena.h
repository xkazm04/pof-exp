#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGEncounterArena.generated.h"

class AARPGCoverPoint;
class UBoxComponent;

/** Elevation tier within an arena. */
UENUM(BlueprintType)
enum class EArenaTier : uint8
{
	Ground     UMETA(DisplayName = "Ground Level"),
	Elevated   UMETA(DisplayName = "Elevated Platform"),
	High       UMETA(DisplayName = "High Ground")
};

/** A tactical position within the arena (pre-defined by designer). */
USTRUCT(BlueprintType)
struct FArenaTacticalPoint
{
	GENERATED_BODY()

	/** World location of this tactical point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical")
	FVector Location = FVector::ZeroVector;

	/** Elevation tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical")
	EArenaTier Tier = EArenaTier::Ground;

	/** Whether this is a good ranged attack position. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical")
	bool bGoodForRanged = false;

	/** Whether this position has nearby cover. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical")
	bool bHasNearbyCover = false;

	/** Whether this is a flanking position relative to the arena center. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical")
	bool bFlankingPosition = false;
};

/**
 * Defines a combat arena with tactical information for encounter design.
 *
 * Place in the world to mark encounter spaces. Automatically discovers
 * nearby CoverPoints and provides queries for AI to find tactical positions
 * (elevation advantages, flanking routes, cover, etc.).
 */
UCLASS()
class POF_API AARPGEncounterArena : public AActor
{
	GENERATED_BODY()

public:
	AARPGEncounterArena();

	/** Scan the arena for cover points and build tactical data. */
	UFUNCTION(BlueprintCallable, Category = "LevelDesign|Arena")
	void ScanArena();

	/** Get all cover points within this arena. */
	UFUNCTION(BlueprintPure, Category = "LevelDesign|Arena")
	TArray<AARPGCoverPoint*> GetCoverPoints() const;

	/** Get available (unclaimed) cover points. */
	UFUNCTION(BlueprintPure, Category = "LevelDesign|Arena")
	TArray<AARPGCoverPoint*> GetAvailableCoverPoints() const;

	/** Find the best cover point that provides cover from a specific attacker. */
	UFUNCTION(BlueprintPure, Category = "LevelDesign|Arena")
	AARPGCoverPoint* FindBestCoverFrom(const FVector& AttackerPosition, const FVector& DefenderPosition) const;

	/** Get tactical points at a specific elevation tier. */
	UFUNCTION(BlueprintPure, Category = "LevelDesign|Arena")
	TArray<FArenaTacticalPoint> GetPointsByTier(EArenaTier Tier) const;

	/** Get a flanking position relative to the target. */
	UFUNCTION(BlueprintPure, Category = "LevelDesign|Arena")
	bool FindFlankingPosition(const FVector& TargetPos, const FVector& AttackerPos, FVector& OutFlankPos) const;

	/** Get the arena center (for AI positioning calculations). */
	UFUNCTION(BlueprintPure, Category = "LevelDesign|Arena")
	FVector GetArenaCenter() const { return GetActorLocation(); }

	/** Get the arena radius. */
	UFUNCTION(BlueprintPure, Category = "LevelDesign|Arena")
	float GetArenaRadius() const;

protected:
	virtual void BeginPlay() override;

	/** Arena bounds for scanning. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena")
	TObjectPtr<UBoxComponent> ArenaBounds;

	/** Pre-defined tactical points (designer-placed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Tactical")
	TArray<FArenaTacticalPoint> TacticalPoints;

	/** Height threshold for "elevated" tier (above arena center). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Tactical", meta = (ClampMin = "50"))
	float ElevatedThreshold = 150.f;

	/** Height threshold for "high ground" tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Tactical", meta = (ClampMin = "100"))
	float HighGroundThreshold = 300.f;

	/** Whether to auto-scan on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	bool bAutoScan = true;

private:
	UPROPERTY()
	TArray<TWeakObjectPtr<AARPGCoverPoint>> DiscoveredCoverPoints;
};

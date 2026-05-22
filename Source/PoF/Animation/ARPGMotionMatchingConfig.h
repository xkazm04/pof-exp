#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ARPGMotionMatchingConfig.generated.h"

class UAnimSequence;

/**
 * Single entry in a motion matching database — an animation clip
 * with tags for filtering and cost weights for matching quality.
 */
USTRUCT(BlueprintType)
struct FMotionMatchEntry
{
	GENERATED_BODY()

	/** The animation sequence for this entry. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatch")
	TObjectPtr<UAnimSequence> Animation;

	/** Tags for filtering (e.g., "Idle", "Walk", "Run", "Turn"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatch")
	TArray<FName> Tags;

	/** Whether this clip has root motion baked in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatch")
	bool bHasRootMotion = false;

	/** Whether this is a looping clip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatch")
	bool bIsLooping = false;

	/** Priority (higher wins when multiple clips match equally well). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatch", meta = (ClampMin = "0"))
	int32 Priority = 0;
};

/**
 * Cost function weights for motion matching — controls how the matcher
 * scores candidate poses against the current character state.
 */
USTRUCT(BlueprintType)
struct FMotionMatchCostWeights
{
	GENERATED_BODY()

	/** Weight for matching the character's current trajectory (position over next N frames). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weights", meta = (ClampMin = "0.0"))
	float TrajectoryPositionWeight = 1.f;

	/** Weight for matching the character's desired facing direction. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weights", meta = (ClampMin = "0.0"))
	float TrajectoryFacingWeight = 0.5f;

	/** Weight for matching the current pose (bone positions/velocities). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weights", meta = (ClampMin = "0.0"))
	float PoseWeight = 1.f;

	/** Penalty multiplied by the distance between current and candidate time to discourage small jumps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weights", meta = (ClampMin = "0.0"))
	float ContinuityCostWeight = 0.2f;
};

/**
 * Data asset that configures a motion matching animation database.
 *
 * In UE 5.7+ with the experimental Motion Matching plugin, this data asset
 * feeds into a PoseSearch schema + database. For projects without the plugin,
 * this serves as a data-driven animation catalog that can be queried by
 * custom chooser logic in the AnimInstance.
 *
 * Usage in AnimBP:
 *   1. Reference this data asset in your AnimInstance
 *   2. Filter entries by tags for the current state
 *   3. Use cost weights to evaluate best-match candidates
 *   4. Play the winning animation at the matched time offset
 */
UCLASS(BlueprintType)
class POF_API UARPGMotionMatchingConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** All animation entries in this motion database. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	TArray<FMotionMatchEntry> Entries;

	/** Cost function weights for pose evaluation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	FMotionMatchCostWeights CostWeights;

	/** Number of future trajectory samples for matching (each at TrajectoryInterval apart). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|Trajectory", meta = (ClampMin = "1", ClampMax = "10"))
	int32 TrajectorySampleCount = 3;

	/** Time interval in seconds between trajectory samples. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching|Trajectory", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float TrajectoryInterval = 0.2f;

	/** Minimum time (seconds) to stay in a clip before allowing a new match. Prevents flickering. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching", meta = (ClampMin = "0.0"))
	float MinResidenceTime = 0.1f;

	/** Blend time when transitioning between matched clips. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MotionMatching", meta = (ClampMin = "0.0"))
	float BlendTime = 0.2f;

	/** Filter entries by tag. Returns all entries that contain the given tag. */
	UFUNCTION(BlueprintPure, Category = "MotionMatching")
	TArray<FMotionMatchEntry> GetEntriesWithTag(FName Tag) const;

	/** Filter entries that match ALL of the given tags. */
	UFUNCTION(BlueprintPure, Category = "MotionMatching")
	TArray<FMotionMatchEntry> GetEntriesWithAllTags(const TArray<FName>& Tags) const;
};

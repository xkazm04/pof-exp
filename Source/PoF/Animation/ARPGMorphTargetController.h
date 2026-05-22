#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ARPGMorphTargetController.generated.h"

class USkeletalMeshComponent;

/**
 * A single morph target blend profile — a named preset of morph target weights
 * that can be blended to/from (e.g., "Damaged", "Angry", "Exhausted").
 */
USTRUCT(BlueprintType)
struct FMorphTargetProfile
{
	GENERATED_BODY()

	/** Display name for this profile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MorphTarget")
	FName ProfileName;

	/** Morph target name -> weight mappings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MorphTarget")
	TMap<FName, float> MorphWeights;
};

/**
 * Active morph blend — tracks an in-progress blend toward a target profile.
 */
USTRUCT()
struct FActiveMorphBlend
{
	GENERATED_BODY()

	/** The profile being blended toward. */
	FMorphTargetProfile TargetProfile;

	/** Current blend alpha (0 = not applied, 1 = fully applied). */
	float CurrentAlpha = 0.f;

	/** Target alpha to blend toward. */
	float TargetAlpha = 1.f;

	/** Blend speed (units per second). */
	float BlendSpeed = 2.f;

	/** Whether this blend should be removed after reaching target alpha of 0. */
	bool bPendingRemoval = false;
};

/**
 * Component that manages morph target (blend shape) weights on a skeletal mesh.
 *
 * Features:
 *   - Named profiles: define presets of morph target weights (e.g., facial expressions,
 *     damage states, fatigue) as data, then blend to/from them at runtime.
 *   - Smooth blending: all transitions interpolate over time for natural results.
 *   - Additive stacking: multiple profiles can be active simultaneously with their
 *     weights composited additively (clamped to 0-1 per morph target).
 *   - Health-driven damage morphs: optional auto-drive morph targets based on health %.
 *
 * Attach to any actor with a USkeletalMeshComponent.
 */
UCLASS(ClassGroup = (Animation), meta = (BlueprintSpawnableComponent))
class POF_API UARPGMorphTargetController : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGMorphTargetController();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Blend to a named profile over time. If already active, updates the target alpha. */
	UFUNCTION(BlueprintCallable, Category = "MorphTarget")
	void BlendToProfile(FName ProfileName, float Alpha = 1.f, float BlendSpeed = 2.f);

	/** Blend a profile out and remove it. */
	UFUNCTION(BlueprintCallable, Category = "MorphTarget")
	void BlendOutProfile(FName ProfileName, float BlendSpeed = 2.f);

	/** Set a single morph target weight directly (no profile, no blending). */
	UFUNCTION(BlueprintCallable, Category = "MorphTarget")
	void SetMorphTargetDirect(FName MorphTargetName, float Weight);

	/** Clear all active profiles and reset all morph targets to 0. */
	UFUNCTION(BlueprintCallable, Category = "MorphTarget")
	void ResetAllMorphTargets();

	/** Predefined morph target profiles (configured in editor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MorphTarget|Profiles")
	TArray<FMorphTargetProfile> Profiles;

	/**
	 * If set, this morph target is driven by 1 - HealthRatio,
	 * creating progressive damage appearance as health drops.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MorphTarget|Health")
	FName DamageMorphTarget;

	/** Whether to auto-drive the damage morph based on health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MorphTarget|Health")
	bool bAutoDriveDamageMorph = false;

private:
	/** Find a profile by name from the Profiles array. */
	const FMorphTargetProfile* FindProfile(FName ProfileName) const;

	/** Apply all active blends to the mesh. */
	void ApplyMorphWeights();

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> TargetMesh;

	/** Currently active morph blend operations. */
	TArray<FActiveMorphBlend> ActiveBlends;

	/** Accumulated morph weights from all active blends (computed each frame). */
	TMap<FName, float> CompositeWeights;
};

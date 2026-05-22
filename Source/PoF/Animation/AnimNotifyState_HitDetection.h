#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_HitDetection.generated.h"

/**
 * Notify state that performs sphere traces from a weapon socket each tick.
 *
 * NotifyBegin: enables hit detection flag, clears the already-hit set.
 * NotifyTick:  sphere traces from the weapon socket, filters duplicates via TSet,
 *              sends Event.MeleeHit gameplay event to the owner's ASC for each new hit.
 * NotifyEnd:   disables hit detection flag, clears the already-hit set.
 *
 * The actual damage application is handled by the active gameplay ability
 * (GA_MeleeAttack) which listens for Event.MeleeHit.
 */
UCLASS(DisplayName = "ARPG Hit Detection")
class POF_API UAnimNotifyState_HitDetection : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** Socket name on the skeletal mesh to trace from (e.g., "weapon_r", "hand_r"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitDetection")
	FName WeaponSocketName = TEXT("weapon_r");

	/** Radius of the sphere trace. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitDetection", meta = (ClampMin = "1.0"))
	float TraceRadius = 30.f;

	/** Length of the trace forward from the socket (along socket forward direction). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitDetection", meta = (ClampMin = "0.0"))
	float TraceLength = 80.f;

	/** Collision channel to trace against. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitDetection")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	/**
	 * Optional tip socket for multi-point tracing along the weapon.
	 * When set, traces between WeaponSocketName (base) and TipSocketName (tip)
	 * for much better hit coverage on long weapons.
	 * If NAME_None, falls back to the single forward-trace from WeaponSocketName.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitDetection")
	FName TipSocketName = NAME_None;

	/**
	 * Number of intermediate trace points between base and tip sockets.
	 * Only used when TipSocketName is set. 0 = trace only base-to-tip.
	 * Higher values give better coverage for fast swings at the cost of more traces.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitDetection", meta = (ClampMin = "0", ClampMax = "8", EditCondition = "TipSocketName != NAME_None"))
	int32 IntermediateTracePoints = 2;

	/** Draw debug traces in non-shipping builds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitDetection")
	bool bDrawDebug = false;

private:
	/** Actors already hit during this swing — prevents duplicate hits. */
	UPROPERTY(Transient)
	TSet<TObjectPtr<AActor>> AlreadyHitActors;

	/** Previous frame's socket positions for frame-to-frame sweep (reduces missed hits at high speed). */
	FVector PrevBaseLocation = FVector::ZeroVector;
	FVector PrevTipLocation = FVector::ZeroVector;
	bool bHasPreviousLocations = false;
};

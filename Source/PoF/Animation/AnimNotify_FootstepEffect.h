#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_FootstepEffect.generated.h"

class USoundBase;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class EFootstepFoot : uint8
{
	Left,
	Right
};

/**
 * Surface-aware footstep notify that plays sound and VFX based on the
 * physical material under the character's foot.
 *
 * Performs a line trace downward from the foot socket to detect the surface,
 * reads the physical material's SurfaceType, and plays the corresponding
 * sound/VFX from the configured arrays.
 */
UCLASS(DisplayName = "ARPG Footstep Effect")
class POF_API UAnimNotify_FootstepEffect : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITORONLY_DATA
	virtual FLinearColor GetEditorColor() override { return FLinearColor(0.6f, 0.4f, 0.2f, 1.f); }
#endif

	/** Which foot this notify is for (determines socket name). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	EFootstepFoot Foot = EFootstepFoot::Left;

	/** Socket name for left foot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	FName LeftFootSocket = TEXT("foot_l");

	/** Socket name for right foot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep")
	FName RightFootSocket = TEXT("foot_r");

	/** Default sound when no surface-specific sound is found. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Sound")
	TObjectPtr<USoundBase> DefaultSound;

	/** Sounds mapped by surface type index (EPhysicalSurface enum value). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Sound")
	TMap<TEnumAsByte<EPhysicalSurface>, TObjectPtr<USoundBase>> SurfaceSounds;

	/** Default VFX when no surface-specific VFX is found. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|VFX")
	TObjectPtr<UNiagaraSystem> DefaultVFX;

	/** VFX mapped by surface type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|VFX")
	TMap<TEnumAsByte<EPhysicalSurface>, TObjectPtr<UNiagaraSystem>> SurfaceVFX;

	/** Volume multiplier for footstep sounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep|Sound", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float VolumeMultiplier = 1.f;

	/** Trace distance downward from foot socket. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Footstep", meta = (ClampMin = "10.0"))
	float TraceDistance = 50.f;
};

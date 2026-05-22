#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_CameraShake.generated.h"

class UCameraShakeBase;

/**
 * Triggers a camera shake on the owning player's camera.
 * Place on attack impact frames, heavy landings, or hit reactions
 * for visceral combat feedback.
 */
UCLASS(DisplayName = "ARPG Camera Shake")
class POF_API UAnimNotify_CameraShake : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITORONLY_DATA
	virtual FLinearColor GetEditorColor() override { return FLinearColor(1.f, 0.4f, 0.f, 1.f); }
#endif

	/** Camera shake class to play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake")
	TSubclassOf<UCameraShakeBase> ShakeClass;

	/** Scale applied to the shake intensity. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake", meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float Scale = 1.f;

	/** If true, only shakes the camera of the player who owns this character. If false, shakes all nearby players. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake")
	bool bOwnerOnly = true;

	/** Inner radius for world-space camera shake (0 = epicenter). Only used when bOwnerOnly is false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake", meta = (ClampMin = "0.0", EditCondition = "!bOwnerOnly"))
	float InnerRadius = 0.f;

	/** Outer radius for world-space camera shake. Only used when bOwnerOnly is false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CameraShake", meta = (ClampMin = "0.0", EditCondition = "!bOwnerOnly"))
	float OuterRadius = 500.f;
};

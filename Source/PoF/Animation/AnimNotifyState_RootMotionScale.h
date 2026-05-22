#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_RootMotionScale.generated.h"

/**
 * Notify state that scales root motion translation and rotation during
 * its active window.
 *
 * Use cases:
 *   - Scale down root motion during a montage's windup phase
 *   - Amplify forward lunge distance on heavy attacks
 *   - Zero out lateral root motion on directional dodges
 *
 * The scale is applied multiplicatively to the character's root motion
 * via SetAnimRootMotionTranslationScale on the owning character.
 * Original scale is restored on NotifyEnd.
 */
UCLASS(DisplayName = "ARPG Root Motion Scale")
class POF_API UAnimNotifyState_RootMotionScale : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITORONLY_DATA
	virtual FLinearColor GetEditorColor() override { return FLinearColor(1.f, 0.6f, 0.f, 1.f); }
#endif

	/** Scale factor for root motion translation (1.0 = unchanged, 0.0 = no translation). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RootMotion", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float TranslationScale = 1.f;

private:
	/** Cached original scale to restore on NotifyEnd. */
	float OriginalScale = 1.f;
};

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_DodgeIFrame.generated.h"

/**
 * Notify state that marks the iframe window on a dodge/roll montage.
 *
 * Fires `Event.Dodge.IFrame.Begin` on NotifyBegin and `Event.Dodge.IFrame.End`
 * on NotifyEnd via gameplay events. GA_Dodge listens for these to toggle the
 * State.Invulnerable tag — keeping the timing data in the montage where
 * animators control it, not hardcoded in the ability.
 *
 * Sibling class: AnimNotifyState_ComboWindow (same pattern).
 */
UCLASS(DisplayName = "ARPG Dodge IFrame (State)")
class POF_API UAnimNotifyState_DodgeIFrame : public UAnimNotifyState
{
    GENERATED_BODY()

public:
    virtual FString GetNotifyName_Implementation() const override;
    virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
    virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITORONLY_DATA
    virtual FLinearColor GetEditorColor() override { return FLinearColor(1.f, 0.4f, 0.8f, 1.f); }
#endif
};

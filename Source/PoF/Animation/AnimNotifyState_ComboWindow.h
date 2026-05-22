#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_ComboWindow.generated.h"

/**
 * Notify state that opens the combo input window on NotifyBegin
 * and closes it on NotifyEnd.
 *
 * This is the preferred notify for combo windows — just drag the
 * duration bar in the montage timeline to define the input window.
 * Sends Event.Combo.Open / Event.Combo.Close gameplay events
 * so GA_MeleeAttack can react.
 */
UCLASS(DisplayName = "ARPG Combo Window (State)")
class POF_API UAnimNotifyState_ComboWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITORONLY_DATA
	virtual FLinearColor GetEditorColor() override { return FLinearColor(0.f, 0.8f, 0.2f, 1.f); }
#endif
};

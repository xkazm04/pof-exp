#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ComboWindow.generated.h"

/**
 * Instant notify that toggles the combo input window on the owning character.
 * Place two of these in a montage: one with bOpenWindow=true, one with bOpenWindow=false.
 *
 * For a more designer-friendly alternative, use UAnimNotifyState_ComboWindow
 * which automatically opens on NotifyBegin and closes on NotifyEnd.
 */
UCLASS(DisplayName = "ARPG Combo Window")
class POF_API UAnimNotify_ComboWindow : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAnimNotify_ComboWindow();

	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITORONLY_DATA
	virtual FLinearColor GetEditorColor() override;
#endif

	/** If true, opens the combo window. If false, closes it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	bool bOpenWindow = true;
};

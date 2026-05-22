#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotify_GameplayEvent.generated.h"

/**
 * Generic anim notify that sends a gameplay event with a configurable tag
 * to the owning actor's AbilitySystemComponent.
 *
 * Use this for designer-driven montage events that don't need custom C++ logic:
 * ability triggers, state changes, audio/VFX cues via GAS, etc.
 */
UCLASS(DisplayName = "ARPG Gameplay Event")
class POF_API UAnimNotify_GameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

#if WITH_EDITORONLY_DATA
	virtual FLinearColor GetEditorColor() override { return FLinearColor(0.8f, 0.2f, 0.8f, 1.f); }
#endif

	/** The gameplay event tag to send. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayEvent")
	FGameplayTag EventTag;

	/** Optional magnitude value passed in the event payload (e.g., damage multiplier, intensity). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayEvent")
	float EventMagnitude = 0.f;
};

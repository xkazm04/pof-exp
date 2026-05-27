#include "Animation/AnimNotifyState_DodgeIFrame.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

namespace
{
    const FGameplayTag DodgeIFrameBeginTag =
        FGameplayTag::RequestGameplayTag(TEXT("Event.Dodge.IFrame.Begin"), /*ErrorIfNotFound*/ false);
    const FGameplayTag DodgeIFrameEndTag =
        FGameplayTag::RequestGameplayTag(TEXT("Event.Dodge.IFrame.End"), /*ErrorIfNotFound*/ false);

    void SendDodgeEvent(USkeletalMeshComponent* MeshComp, FGameplayTag EventTag)
    {
        if (!MeshComp || !EventTag.IsValid()) return;
        AActor* Owner = MeshComp->GetOwner();
        if (!Owner) return;
        FGameplayEventData Payload;
        Payload.Instigator = Owner;
        Payload.EventTag = EventTag;
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, Payload);
    }
}

FString UAnimNotifyState_DodgeIFrame::GetNotifyName_Implementation() const
{
    return TEXT("DodgeIFrame");
}

void UAnimNotifyState_DodgeIFrame::NotifyBegin(USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* /*Animation*/, float /*TotalDuration*/,
    const FAnimNotifyEventReference& /*EventReference*/)
{
    SendDodgeEvent(MeshComp, DodgeIFrameBeginTag);
}

void UAnimNotifyState_DodgeIFrame::NotifyEnd(USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* /*Animation*/, const FAnimNotifyEventReference& /*EventReference*/)
{
    SendDodgeEvent(MeshComp, DodgeIFrameEndTag);
}

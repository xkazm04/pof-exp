#include "Animation/AnimNotify_GameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

FString UAnimNotify_GameplayEvent::GetNotifyName_Implementation() const
{
	if (EventTag.IsValid())
	{
		return FString::Printf(TEXT("Event: %s"), *EventTag.ToString());
	}
	return TEXT("Event: None");
}

void UAnimNotify_GameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !EventTag.IsValid())
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			FGameplayEventData Payload;
			Payload.EventTag = EventTag;
			Payload.Instigator = Owner;
			Payload.EventMagnitude = EventMagnitude;
			ASC->HandleGameplayEvent(EventTag, &Payload);
		}
	}
}

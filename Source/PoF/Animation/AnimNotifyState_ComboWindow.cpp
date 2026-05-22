#include "Animation/AnimNotifyState_ComboWindow.h"
#include "Character/ARPGCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ARPGGameplayTags.h"

FString UAnimNotifyState_ComboWindow::GetNotifyName_Implementation() const
{
	return TEXT("Combo Window");
}

void UAnimNotifyState_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp) return;

	AARPGCharacterBase* Character = Cast<AARPGCharacterBase>(MeshComp->GetOwner());
	if (!Character) return;

	Character->OpenComboWindow();

	if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
	{
		FGameplayEventData Payload;
		Payload.Instigator = Character;
		Payload.EventTag = ARPGGameplayTags::Event_Combo_Open;
		ASC->HandleGameplayEvent(Payload.EventTag, &Payload);
	}
}

void UAnimNotifyState_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	AARPGCharacterBase* Character = Cast<AARPGCharacterBase>(MeshComp->GetOwner());
	if (!Character) return;

	Character->CloseComboWindow();

	if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
	{
		FGameplayEventData Payload;
		Payload.Instigator = Character;
		Payload.EventTag = ARPGGameplayTags::Event_Combo_Close;
		ASC->HandleGameplayEvent(Payload.EventTag, &Payload);
	}
}

#include "Animation/AnimNotify_ComboWindow.h"
#include "Character/ARPGCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ARPGGameplayTags.h"

UAnimNotify_ComboWindow::UAnimNotify_ComboWindow()
{
}

#if WITH_EDITORONLY_DATA
FLinearColor UAnimNotify_ComboWindow::GetEditorColor()
{
	return bOpenWindow ? FLinearColor::Green : FLinearColor::Red;
}
#endif

FString UAnimNotify_ComboWindow::GetNotifyName_Implementation() const
{
	return bOpenWindow ? TEXT("Combo Open") : TEXT("Combo Close");
}

void UAnimNotify_ComboWindow::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AARPGCharacterBase* Character = Cast<AARPGCharacterBase>(MeshComp->GetOwner());
	if (!Character)
	{
		return;
	}

	if (bOpenWindow)
	{
		Character->OpenComboWindow();
	}
	else
	{
		Character->CloseComboWindow();
	}

	// Send gameplay event so GA_MeleeAttack can react via WaitGameplayEvent
	if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
	{
		FGameplayEventData Payload;
		Payload.Instigator = Character;
		Payload.EventTag = bOpenWindow ? ARPGGameplayTags::Event_Combo_Open : ARPGGameplayTags::Event_Combo_Close;
		ASC->HandleGameplayEvent(Payload.EventTag, &Payload);
	}
}

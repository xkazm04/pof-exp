#include "Animation/AnimNotifyState_RootMotionScale.h"
#include "GameFramework/Character.h"

FString UAnimNotifyState_RootMotionScale::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("RootMotion x%.1f"), TranslationScale);
}

void UAnimNotifyState_RootMotionScale::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp) return;

	ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner());
	if (!Character) return;

	OriginalScale = Character->GetAnimRootMotionTranslationScale();
	Character->SetAnimRootMotionTranslationScale(TranslationScale);
}

void UAnimNotifyState_RootMotionScale::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	ACharacter* Character = Cast<ACharacter>(MeshComp->GetOwner());
	if (!Character) return;

	Character->SetAnimRootMotionTranslationScale(OriginalScale);
}

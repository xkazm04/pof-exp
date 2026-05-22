#include "Animation/AnimNotify_CameraShake.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraShakeBase.h"

FString UAnimNotify_CameraShake::GetNotifyName_Implementation() const
{
	if (ShakeClass)
	{
		return FString::Printf(TEXT("Shake: %s"), *ShakeClass->GetName());
	}
	return TEXT("Shake: None");
}

void UAnimNotify_CameraShake::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !ShakeClass)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	if (bOwnerOnly)
	{
		// Shake only the owning player's camera
		APawn* Pawn = Cast<APawn>(Owner);
		APlayerController* PC = Pawn ? Pawn->GetController<APlayerController>() : nullptr;
		if (!PC)
		{
			// If the owner isn't a player pawn (e.g. enemy), shake the local player's camera
			PC = Owner->GetWorld() ? Owner->GetWorld()->GetFirstPlayerController() : nullptr;
		}

		if (PC && PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraShake(ShakeClass, Scale);
		}
	}
	else
	{
		// World-space shake affecting all nearby players
		UGameplayStatics::PlayWorldCameraShake(
			Owner->GetWorld(),
			ShakeClass,
			Owner->GetActorLocation(),
			InnerRadius,
			OuterRadius,
			Scale
		);
	}
}

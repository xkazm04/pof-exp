#include "Animation/AnimNotify_ARPGPlaySound.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

FString UAnimNotify_ARPGPlaySound::GetNotifyName_Implementation() const
{
	if (Sound)
	{
		return FString::Printf(TEXT("Sound: %s"), *Sound->GetName());
	}
	return TEXT("Sound: None");
}

void UAnimNotify_ARPGPlaySound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !Sound)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	if (bAttachToSocket)
	{
		UGameplayStatics::SpawnSoundAttached(
			Sound,
			MeshComp,
			SocketName,
			FVector::ZeroVector,
			EAttachLocation::SnapToTarget,
			true, // bStopWhenAttachedToDestroyed
			VolumeMultiplier,
			PitchMultiplier
		);
	}
	else
	{
		const FVector Location = (SocketName != NAME_None)
			? MeshComp->GetSocketLocation(SocketName)
			: Owner->GetActorLocation();

		UGameplayStatics::PlaySoundAtLocation(
			Owner->GetWorld(),
			Sound,
			Location,
			VolumeMultiplier,
			PitchMultiplier
		);
	}
}

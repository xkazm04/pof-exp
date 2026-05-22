#include "Animation/AnimNotify_SpawnVFX.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

FString UAnimNotify_SpawnVFX::GetNotifyName_Implementation() const
{
	if (NiagaraEffect)
	{
		return FString::Printf(TEXT("VFX: %s"), *NiagaraEffect->GetName());
	}
	return TEXT("VFX: None");
}

void UAnimNotify_SpawnVFX::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !NiagaraEffect)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	if (bAttachToSocket && SocketName != NAME_None)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraEffect,
			MeshComp,
			SocketName,
			LocationOffset,
			RotationOffset,
			Scale,
			EAttachLocation::KeepRelativeOffset,
			true, // bAutoDestroy
			ENCPoolMethod::None
		);
	}
	else
	{
		const FTransform MeshTransform = (SocketName != NAME_None)
			? MeshComp->GetSocketTransform(SocketName)
			: Owner->GetActorTransform();

		const FVector SpawnLocation = MeshTransform.GetLocation() + MeshTransform.GetRotation().RotateVector(LocationOffset);
		const FRotator SpawnRotation = MeshTransform.GetRotation().Rotator() + RotationOffset;

		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			Owner->GetWorld(),
			NiagaraEffect,
			SpawnLocation,
			SpawnRotation,
			Scale,
			true, // bAutoDestroy
			true, // bAutoActivate
			ENCPoolMethod::None
		);
	}
}

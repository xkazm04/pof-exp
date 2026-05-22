#include "Animation/AnimNotify_FootstepEffect.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

FString UAnimNotify_FootstepEffect::GetNotifyName_Implementation() const
{
	return Foot == EFootstepFoot::Left ? TEXT("Footstep L") : TEXT("Footstep R");
}

void UAnimNotify_FootstepEffect::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	UWorld* World = Owner->GetWorld();
	if (!World)
	{
		return;
	}

	// Get foot socket location
	const FName SocketName = (Foot == EFootstepFoot::Left) ? LeftFootSocket : RightFootSocket;
	const FVector FootLocation = MeshComp->GetSocketLocation(SocketName);

	// Line trace downward to detect surface material
	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Owner);
	QueryParams.bReturnPhysicalMaterial = true;

	const FVector TraceStart = FootLocation;
	const FVector TraceEnd = FootLocation - FVector(0.f, 0.f, TraceDistance);

	EPhysicalSurface SurfaceType = SurfaceType_Default;
	FVector ImpactLocation = FootLocation;

	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		ImpactLocation = Hit.ImpactPoint;
		if (Hit.PhysMaterial.IsValid())
		{
			SurfaceType = Hit.PhysMaterial->SurfaceType;
		}
	}

	// Play sound
	USoundBase* SoundToPlay = DefaultSound;
	if (const TObjectPtr<USoundBase>* Found = SurfaceSounds.Find(SurfaceType))
	{
		SoundToPlay = *Found;
	}

	if (SoundToPlay)
	{
		UGameplayStatics::PlaySoundAtLocation(World, SoundToPlay, ImpactLocation, VolumeMultiplier);
	}

	// Spawn VFX
	UNiagaraSystem* VFXToSpawn = DefaultVFX;
	if (const TObjectPtr<UNiagaraSystem>* Found = SurfaceVFX.Find(SurfaceType))
	{
		VFXToSpawn = *Found;
	}

	if (VFXToSpawn)
	{
		const FRotator SpawnRotation = Hit.bBlockingHit
			? Hit.ImpactNormal.Rotation()
			: FRotator(-90.f, 0.f, 0.f);

		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			VFXToSpawn,
			ImpactLocation,
			SpawnRotation,
			FVector::OneVector,
			true,
			true,
			ENCPoolMethod::AutoRelease
		);
	}
}

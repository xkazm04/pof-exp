#include "Audio/ARPGCombatAudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

UARPGCombatAudioComponent::UARPGCombatAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UARPGCombatAudioComponent::PlayCombatSound(ECombatSoundType SoundType)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	for (const FCombatSoundEntry& Entry : CombatSounds)
	{
		if (Entry.SoundType == SoundType)
		{
			USoundBase* Sound = GetRandomSound(Entry.SoundVariations);
			if (!Sound)
			{
				return;
			}

			if (Entry.AttachSocket != NAME_None)
			{
				USkeletalMeshComponent* Mesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
				if (Mesh)
				{
					const float Pitch = 1.f + FMath::RandRange(-Entry.PitchVariation, Entry.PitchVariation);
					UGameplayStatics::SpawnSoundAttached(Sound, Mesh, Entry.AttachSocket, FVector::ZeroVector, EAttachLocation::SnapToTarget, true, Entry.VolumeMultiplier, Pitch);
					return;
				}
			}

			PlaySoundWithVariation(Sound, Owner->GetActorLocation(), Entry.VolumeMultiplier, Entry.PitchVariation);
			return;
		}
	}
}

void UARPGCombatAudioComponent::PlayImpactSoundAtLocation(FVector Location, EPhysicalSurface SurfaceType)
{
	const FSurfaceSoundEntry* Entry = FindSurfaceEntry(SurfaceType);
	if (!Entry)
	{
		Entry = FindSurfaceEntry(EPhysicalSurface::SurfaceType_Default);
	}
	if (!Entry)
	{
		return;
	}

	USoundBase* Sound = GetRandomSound(Entry->ImpactSounds);
	if (Sound)
	{
		PlaySoundWithVariation(Sound, Location, 1.f, 0.15f);
	}
}

void UARPGCombatAudioComponent::PlayFootstepSound(EPhysicalSurface SurfaceType)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const FSurfaceSoundEntry* Entry = FindSurfaceEntry(SurfaceType);
	if (!Entry)
	{
		Entry = FindSurfaceEntry(EPhysicalSurface::SurfaceType_Default);
	}
	if (!Entry || Entry->FootstepSounds.Num() == 0)
	{
		return;
	}

	// Avoid repeating the same footstep
	int32 Index = FMath::RandRange(0, Entry->FootstepSounds.Num() - 1);
	if (Index == LastFootstepIndex && Entry->FootstepSounds.Num() > 1)
	{
		Index = (Index + 1) % Entry->FootstepSounds.Num();
	}
	LastFootstepIndex = Index;

	USoundBase* Sound = Entry->FootstepSounds[Index];
	if (Sound)
	{
		PlaySoundWithVariation(Sound, Owner->GetActorLocation(), 0.6f, 0.08f);
	}
}

void UARPGCombatAudioComponent::PlaySoundWithVariation(USoundBase* Sound, FVector Location, float BaseVolume, float PitchVar)
{
	if (!Sound)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	const float Pitch = 1.f + FMath::RandRange(-PitchVar, PitchVar);
	UGameplayStatics::PlaySoundAtLocation(Owner->GetWorld(), Sound, Location, FRotator::ZeroRotator, BaseVolume, Pitch);
}

USoundBase* UARPGCombatAudioComponent::GetRandomSound(const TArray<TObjectPtr<USoundBase>>& Sounds) const
{
	if (Sounds.Num() == 0)
	{
		return nullptr;
	}
	return Sounds[FMath::RandRange(0, Sounds.Num() - 1)];
}

const FSurfaceSoundEntry* UARPGCombatAudioComponent::FindSurfaceEntry(EPhysicalSurface SurfaceType) const
{
	for (const FSurfaceSoundEntry& Entry : SurfaceSounds)
	{
		if (Entry.SurfaceType == SurfaceType)
		{
			return &Entry;
		}
	}
	return nullptr;
}

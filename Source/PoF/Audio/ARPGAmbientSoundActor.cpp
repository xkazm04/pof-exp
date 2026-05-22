#include "Audio/ARPGAmbientSoundActor.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "Sound/SoundAttenuation.h"

AARPGAmbientSoundActor::AARPGAmbientSoundActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;

	AttenuationSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AttenuationSphere"));
	AttenuationSphere->SetSphereRadius(AttenuationRadius);
	AttenuationSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttenuationSphere->SetHiddenInGame(true);
	RootComponent = AttenuationSphere;

	AudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComp->SetupAttachment(RootComponent);
	AudioComp->bAutoActivate = false;
	AudioComp->bOverrideAttenuation = true;
}

void AARPGAmbientSoundActor::BeginPlay()
{
	Super::BeginPlay();

	if (AmbientSound)
	{
		AudioComp->SetSound(AmbientSound);
	}

	// Configure attenuation
	FSoundAttenuationSettings AttSettings;
	AttSettings.bAttenuate = true;
	AttSettings.bSpatialize = true;
	AttSettings.FalloffDistance = AttenuationRadius;
	AttSettings.bEnableOcclusion = bEnableOcclusion;
	AttSettings.OcclusionLowPassFilterFrequency = 2000.f;
	AttSettings.OcclusionVolumeAttenuation = 0.5f;
	AttSettings.OcclusionInterpolationTime = 0.3f;
	AudioComp->AttenuationOverrides = AttSettings;

	AudioComp->SetVolumeMultiplier(VolumeMultiplier);
	AudioComp->SetPitchMultiplier(PitchMultiplier);

	bIsEnabled = bStartEnabled;

	if (bIsEnabled && SoundType == EAmbientSoundType::Looping)
	{
		AudioComp->Play();
	}
	else if (bIsEnabled && SoundType == EAmbientSoundType::RandomInterval)
	{
		ScheduleNextTrigger();
	}
	else if (bIsEnabled && SoundType == EAmbientSoundType::TimeOfDay)
	{
		bTimeOfDayActive = IsWithinActiveHours();
		if (bTimeOfDayActive)
		{
			AudioComp->Play();
		}
	}
}

void AARPGAmbientSoundActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsEnabled)
	{
		return;
	}

	if (SoundType == EAmbientSoundType::RandomInterval)
	{
		UWorld* World = GetWorld();
		if (World && World->GetTimeSeconds() >= NextTriggerTime)
		{
			PlayOneShot();
			ScheduleNextTrigger();
		}
	}
	else if (SoundType == EAmbientSoundType::TimeOfDay)
	{
		const bool bShouldBeActive = IsWithinActiveHours();
		if (bShouldBeActive && !bTimeOfDayActive)
		{
			bTimeOfDayActive = true;
			AudioComp->FadeIn(TimeOfDayFadeDuration, VolumeMultiplier);
		}
		else if (!bShouldBeActive && bTimeOfDayActive)
		{
			bTimeOfDayActive = false;
			AudioComp->FadeOut(TimeOfDayFadeDuration, 0.f);
		}
	}
}

void AARPGAmbientSoundActor::SetEnabled(bool bEnable)
{
	bIsEnabled = bEnable;

	if (!bIsEnabled)
	{
		AudioComp->Stop();
	}
	else if (SoundType == EAmbientSoundType::Looping)
	{
		AudioComp->Play();
	}
	else if (SoundType == EAmbientSoundType::RandomInterval)
	{
		ScheduleNextTrigger();
	}
}

void AARPGAmbientSoundActor::SetVolumeMultiplier(float NewVolume)
{
	VolumeMultiplier = FMath::Clamp(NewVolume, 0.f, 2.f);
	AudioComp->SetVolumeMultiplier(VolumeMultiplier);
}

void AARPGAmbientSoundActor::PlayOneShot()
{
	if (!AmbientSound)
	{
		return;
	}

	const float Pitch = PitchMultiplier + FMath::RandRange(-RandomPitchVariation, RandomPitchVariation);
	AudioComp->SetPitchMultiplier(FMath::Clamp(Pitch, 0.1f, 3.f));
	AudioComp->Play();
}

void AARPGAmbientSoundActor::ScheduleNextTrigger()
{
	UWorld* World = GetWorld();
	if (World)
	{
		NextTriggerTime = World->GetTimeSeconds() + FMath::RandRange(MinInterval, MaxInterval);
	}
}

bool AARPGAmbientSoundActor::IsWithinActiveHours() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// Use game time scaled to 24h cycle (seconds -> hours, 1 game day = 1200 seconds = 20 minutes)
	const float GameTimeSeconds = World->GetTimeSeconds();
	const float GameDayLengthSeconds = 1200.f;
	const float CurrentHour = FMath::Fmod(GameTimeSeconds / GameDayLengthSeconds * 24.f, 24.f);

	if (ActiveStartHour <= ActiveEndHour)
	{
		return CurrentHour >= ActiveStartHour && CurrentHour < ActiveEndHour;
	}
	// Wraps midnight (e.g. 20:00 - 06:00)
	return CurrentHour >= ActiveStartHour || CurrentHour < ActiveEndHour;
}

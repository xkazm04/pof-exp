#include "Audio/ARPGAudioVolume.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Character.h"
#include "Sound/ReverbEffect.h"
#include "AudioDevice.h"

AARPGAudioVolume::AARPGAudioVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	VolumeBox = CreateDefaultSubobject<UBoxComponent>(TEXT("VolumeBox"));
	VolumeBox->SetBoxExtent(VolumeExtent);
	VolumeBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	VolumeBox->SetGenerateOverlapEvents(true);
	RootComponent = VolumeBox;

	AmbientAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("AmbientAudio"));
	AmbientAudioComp->SetupAttachment(RootComponent);
	AmbientAudioComp->bAutoActivate = false;
	AmbientAudioComp->bIsUISound = false;
}

void AARPGAudioVolume::BeginPlay()
{
	Super::BeginPlay();

	VolumeBox->SetBoxExtent(VolumeExtent);
	VolumeBox->OnComponentBeginOverlap.AddDynamic(this, &AARPGAudioVolume::OnVolumeBeginOverlap);
	VolumeBox->OnComponentEndOverlap.AddDynamic(this, &AARPGAudioVolume::OnVolumeEndOverlap);

	if (ReverbPreset != EReverbPreset::Custom)
	{
		ApplyReverbPreset(ReverbPreset);
	}
	else if (ReverbPreset == EReverbPreset::Custom)
	{
		// Build reverb effect from custom settings
		ActiveReverbEffect = NewObject<UReverbEffect>(this, TEXT("ReverbEffect"));
		ActiveReverbEffect->Density = ReverbSettings.Density;
		ActiveReverbEffect->Diffusion = ReverbSettings.Diffusion;
		ActiveReverbEffect->DecayTime = ReverbSettings.DecayTime;
		ActiveReverbEffect->DecayHFRatio = ReverbSettings.DecayHFRatio;
		ActiveReverbEffect->ReflectionsGain = ReverbSettings.ReflectionsGain;
		ActiveReverbEffect->LateGain = ReverbSettings.LateReverbGain;
		ActiveReverbEffect->AirAbsorptionGainHF = ReverbSettings.AirAbsorptionGainHF;
	}

	if (InteriorAmbientSound)
	{
		AmbientAudioComp->SetSound(InteriorAmbientSound);
		AmbientAudioComp->SetVolumeMultiplier(InteriorAmbientVolume);
	}
}

void AARPGAudioVolume::SetReverbPreset(EReverbPreset NewPreset)
{
	ReverbPreset = NewPreset;
	ApplyReverbPreset(NewPreset);
}

void AARPGAudioVolume::OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACharacter* PlayerChar = Cast<ACharacter>(OtherActor);
	if (!PlayerChar || !PlayerChar->IsLocallyControlled())
	{
		return;
	}

	OverlappingPlayerCount++;

	if (InteriorAmbientSound && !AmbientAudioComp->IsPlaying())
	{
		AmbientAudioComp->FadeIn(FadeTime, InteriorAmbientVolume);
	}

	ActivateReverb();
}

void AARPGAudioVolume::OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ACharacter* PlayerChar = Cast<ACharacter>(OtherActor);
	if (!PlayerChar || !PlayerChar->IsLocallyControlled())
	{
		return;
	}

	OverlappingPlayerCount = FMath::Max(0, OverlappingPlayerCount - 1);

	if (OverlappingPlayerCount == 0)
	{
		if (AmbientAudioComp->IsPlaying())
		{
			AmbientAudioComp->FadeOut(FadeTime, 0.f);
		}

		DeactivateReverb();
	}
}

void AARPGAudioVolume::ApplyReverbPreset(EReverbPreset Preset)
{
	if (Preset == EReverbPreset::Custom)
	{
		return;
	}
	ReverbSettings = GetPresetSettings(Preset);

	// Create the reverb effect object from our settings
	ActiveReverbEffect = NewObject<UReverbEffect>(this, TEXT("ReverbEffect"));
	ActiveReverbEffect->Density = ReverbSettings.Density;
	ActiveReverbEffect->Diffusion = ReverbSettings.Diffusion;
	ActiveReverbEffect->DecayTime = ReverbSettings.DecayTime;
	ActiveReverbEffect->DecayHFRatio = ReverbSettings.DecayHFRatio;
	ActiveReverbEffect->ReflectionsGain = ReverbSettings.ReflectionsGain;
	ActiveReverbEffect->LateGain = ReverbSettings.LateReverbGain;
	ActiveReverbEffect->AirAbsorptionGainHF = ReverbSettings.AirAbsorptionGainHF;
}

void AARPGAudioVolume::ActivateReverb()
{
	if (!ActiveReverbEffect || ReverbPreset == EReverbPreset::None)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (FAudioDeviceHandle AudioDevice = World->GetAudioDevice())
	{
		const uint32 UniqueID = GetUniqueID();
		AudioDevice->ActivateReverbEffect(ActiveReverbEffect, FName(*GetName()), Priority, ReverbSettings.Volume, FadeTime);
	}
}

void AARPGAudioVolume::DeactivateReverb()
{
	if (ReverbPreset == EReverbPreset::None)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (FAudioDeviceHandle AudioDevice = World->GetAudioDevice())
	{
		AudioDevice->DeactivateReverbEffect(FName(*GetName()));
	}
}

FARPGReverbSettings AARPGAudioVolume::GetPresetSettings(EReverbPreset Preset)
{
	FARPGReverbSettings Settings;

	switch (Preset)
	{
	case EReverbPreset::SmallRoom:
		Settings.DecayTime = 0.8f;
		Settings.Density = 0.8f;
		Settings.Diffusion = 0.9f;
		Settings.ReflectionsGain = 0.15f;
		Settings.LateReverbGain = 0.8f;
		break;
	case EReverbPreset::LargeHall:
		Settings.DecayTime = 3.5f;
		Settings.Density = 1.0f;
		Settings.Diffusion = 1.0f;
		Settings.ReflectionsGain = 0.08f;
		Settings.LateReverbGain = 1.5f;
		break;
	case EReverbPreset::Cave:
		Settings.DecayTime = 5.0f;
		Settings.Density = 0.6f;
		Settings.Diffusion = 0.4f;
		Settings.ReflectionsGain = 0.2f;
		Settings.LateReverbGain = 1.8f;
		Settings.DecayHFRatio = 0.5f;
		break;
	case EReverbPreset::Forest:
		Settings.DecayTime = 1.2f;
		Settings.Density = 0.3f;
		Settings.Diffusion = 1.0f;
		Settings.ReflectionsGain = 0.02f;
		Settings.LateReverbGain = 0.4f;
		Settings.AirAbsorptionGainHF = 0.95f;
		break;
	case EReverbPreset::Underwater:
		Settings.DecayTime = 8.0f;
		Settings.Density = 1.0f;
		Settings.Diffusion = 1.0f;
		Settings.ReflectionsGain = 0.1f;
		Settings.LateReverbGain = 2.0f;
		Settings.DecayHFRatio = 0.2f;
		Settings.AirAbsorptionGainHF = 0.8f;
		break;
	case EReverbPreset::Cathedral:
		Settings.DecayTime = 6.0f;
		Settings.Density = 1.0f;
		Settings.Diffusion = 1.0f;
		Settings.ReflectionsGain = 0.12f;
		Settings.LateReverbGain = 1.6f;
		Settings.DecayHFRatio = 1.0f;
		break;
	case EReverbPreset::Arena:
		Settings.DecayTime = 4.0f;
		Settings.Density = 0.9f;
		Settings.Diffusion = 0.8f;
		Settings.ReflectionsGain = 0.1f;
		Settings.LateReverbGain = 1.4f;
		break;
	default:
		break;
	}

	return Settings;
}

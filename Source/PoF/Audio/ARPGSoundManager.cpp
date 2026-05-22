#include "Audio/ARPGSoundManager.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundConcurrency.h"
#include "GameFramework/GameUserSettings.h"

bool FARPGSoundPoolEntry::IsAvailable(const UWorld* World) const
{
	return AudioComponent && IsValid(AudioComponent) && !AudioComponent->IsPlaying();
}

static const FString AudioSettingsSection = TEXT("/Script/PoF.AudioSettings");

void UARPGSoundManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	InitializeDefaultVolumes();
	LoadVolumeSettings();

	DefaultConcurrency = NewObject<USoundConcurrency>(this, TEXT("DefaultConcurrency"));
	DefaultConcurrency->Concurrency.MaxCount = 8;
	DefaultConcurrency->Concurrency.bLimitToOwner = false;
	DefaultConcurrency->Concurrency.ResolutionRule = EMaxConcurrentResolutionRule::StopLowestPriority;
	DefaultConcurrency->Concurrency.VolumeScaleMode = EConcurrencyVolumeScaleMode::Default;
}

void UARPGSoundManager::Deinitialize()
{
	for (FARPGSoundPoolEntry& Entry : SoundPool)
	{
		if (Entry.AudioComponent && Entry.AudioComponent->IsPlaying())
		{
			Entry.AudioComponent->Stop();
		}
	}
	SoundPool.Empty();
	Super::Deinitialize();
}

void UARPGSoundManager::InitializeDefaultVolumes()
{
	CategoryVolumes.Add(EARPGSoundCategory::Master, 1.f);
	CategoryVolumes.Add(EARPGSoundCategory::Music, 0.8f);
	CategoryVolumes.Add(EARPGSoundCategory::SFX, 1.f);
	CategoryVolumes.Add(EARPGSoundCategory::Ambient, 0.7f);
	CategoryVolumes.Add(EARPGSoundCategory::UI, 1.f);
	CategoryVolumes.Add(EARPGSoundCategory::Voice, 1.f);
}

void UARPGSoundManager::SetCategoryVolume(EARPGSoundCategory Category, float Volume)
{
	CategoryVolumes.FindOrAdd(Category) = FMath::Clamp(Volume, 0.f, 1.f);
}

float UARPGSoundManager::GetCategoryVolume(EARPGSoundCategory Category) const
{
	const float* Vol = CategoryVolumes.Find(Category);
	return Vol ? *Vol : 1.f;
}

float UARPGSoundManager::GetEffectiveVolume(EARPGSoundCategory Category, float Multiplier) const
{
	const float MasterVol = GetCategoryVolume(EARPGSoundCategory::Master);
	const float CategoryVol = GetCategoryVolume(Category);
	return MasterVol * CategoryVol * Multiplier;
}

UAudioComponent* UARPGSoundManager::PlaySound2D(UObject* WorldContextObject, USoundBase* Sound, EARPGSoundCategory Category, float VolumeMultiplier, float PitchMultiplier)
{
	if (!Sound || !WorldContextObject)
	{
		return nullptr;
	}

	const float EffectiveVolume = GetEffectiveVolume(Category, VolumeMultiplier);
	return UGameplayStatics::SpawnSound2D(WorldContextObject, Sound, EffectiveVolume, PitchMultiplier);
}

UAudioComponent* UARPGSoundManager::PlaySoundAtLocation(UObject* WorldContextObject, USoundBase* Sound, FVector Location, EARPGSoundCategory Category, float VolumeMultiplier, float PitchMultiplier)
{
	if (!Sound || !WorldContextObject)
	{
		return nullptr;
	}

	const float EffectiveVolume = GetEffectiveVolume(Category, VolumeMultiplier);
	return UGameplayStatics::SpawnSoundAtLocation(WorldContextObject, Sound, Location, FRotator::ZeroRotator, EffectiveVolume, PitchMultiplier);
}

UAudioComponent* UARPGSoundManager::PlaySoundAttached(USoundBase* Sound, USceneComponent* AttachTo, FName SocketName, EARPGSoundCategory Category, float VolumeMultiplier, float PitchMultiplier)
{
	if (!Sound || !AttachTo)
	{
		return nullptr;
	}

	const float EffectiveVolume = GetEffectiveVolume(Category, VolumeMultiplier);
	return UGameplayStatics::SpawnSoundAttached(Sound, AttachTo, SocketName, FVector::ZeroVector, EAttachLocation::SnapToTarget, true, EffectiveVolume, PitchMultiplier);
}

UAudioComponent* UARPGSoundManager::GetPooledComponent(UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (!World)
	{
		return nullptr;
	}

	const float CurrentTime = World->GetTimeSeconds();

	// Find available pooled component
	for (FARPGSoundPoolEntry& Entry : SoundPool)
	{
		if (Entry.IsAvailable(World) && (CurrentTime - Entry.LastUsedTime) > PoolRecycleTime)
		{
			Entry.LastUsedTime = CurrentTime;
			return Entry.AudioComponent;
		}
	}

	// Create new if pool not full
	if (SoundPool.Num() < MaxPoolSize)
	{
		UAudioComponent* NewComp = NewObject<UAudioComponent>(WorldContextObject);
		if (NewComp)
		{
			NewComp->bAutoDestroy = false;
			NewComp->bAutoActivate = false;
			NewComp->RegisterComponentWithWorld(World);

			FARPGSoundPoolEntry& Entry = SoundPool.AddDefaulted_GetRef();
			Entry.AudioComponent = NewComp;
			Entry.LastUsedTime = CurrentTime;
			return NewComp;
		}
	}

	return nullptr;
}

void UARPGSoundManager::ReturnToPool(UAudioComponent* Component)
{
	if (!Component)
	{
		return;
	}

	Component->Stop();
	for (FARPGSoundPoolEntry& Entry : SoundPool)
	{
		if (Entry.AudioComponent == Component)
		{
			Entry.LastUsedTime = 0.f;
			return;
		}
	}
}

void UARPGSoundManager::SaveVolumeSettings()
{
	GConfig->SetFloat(*AudioSettingsSection, TEXT("MasterVolume"), GetCategoryVolume(EARPGSoundCategory::Master), GGameUserSettingsIni);
	GConfig->SetFloat(*AudioSettingsSection, TEXT("MusicVolume"), GetCategoryVolume(EARPGSoundCategory::Music), GGameUserSettingsIni);
	GConfig->SetFloat(*AudioSettingsSection, TEXT("SFXVolume"), GetCategoryVolume(EARPGSoundCategory::SFX), GGameUserSettingsIni);
	GConfig->SetFloat(*AudioSettingsSection, TEXT("AmbientVolume"), GetCategoryVolume(EARPGSoundCategory::Ambient), GGameUserSettingsIni);
	GConfig->SetFloat(*AudioSettingsSection, TEXT("UIVolume"), GetCategoryVolume(EARPGSoundCategory::UI), GGameUserSettingsIni);
	GConfig->SetFloat(*AudioSettingsSection, TEXT("VoiceVolume"), GetCategoryVolume(EARPGSoundCategory::Voice), GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

void UARPGSoundManager::LoadVolumeSettings()
{
	auto LoadCategory = [this](EARPGSoundCategory Category, const TCHAR* Key, float Default)
	{
		float Value = Default;
		if (GConfig->GetFloat(*AudioSettingsSection, Key, Value, GGameUserSettingsIni))
		{
			CategoryVolumes.FindOrAdd(Category) = FMath::Clamp(Value, 0.f, 1.f);
		}
	};

	LoadCategory(EARPGSoundCategory::Master, TEXT("MasterVolume"), 1.f);
	LoadCategory(EARPGSoundCategory::Music, TEXT("MusicVolume"), 0.8f);
	LoadCategory(EARPGSoundCategory::SFX, TEXT("SFXVolume"), 1.f);
	LoadCategory(EARPGSoundCategory::Ambient, TEXT("AmbientVolume"), 0.7f);
	LoadCategory(EARPGSoundCategory::UI, TEXT("UIVolume"), 1.f);
	LoadCategory(EARPGSoundCategory::Voice, TEXT("VoiceVolume"), 1.f);
}

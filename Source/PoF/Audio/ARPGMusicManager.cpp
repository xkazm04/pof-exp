#include "Audio/ARPGMusicManager.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UARPGMusicManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Audio components are created lazily when first needed, since no world exists during Initialize
}

void UARPGMusicManager::Deinitialize()
{
	if (PrimaryTrack && PrimaryTrack->IsPlaying())
	{
		PrimaryTrack->Stop();
	}
	if (SecondaryTrack && SecondaryTrack->IsPlaying())
	{
		SecondaryTrack->Stop();
	}
	Super::Deinitialize();
}

void UARPGMusicManager::SetMusicState(EMusicState NewState)
{
	if (NewState == CurrentState)
	{
		return;
	}

	const EMusicState OldState = CurrentState;
	CurrentState = NewState;

	if (NewState == EMusicState::None)
	{
		StopMusic(CrossfadeDuration);
		OnMusicStateChanged.Broadcast(OldState, NewState);
		return;
	}

	const FMusicTrackEntry* Entry = TrackMap.Find(NewState);
	if (Entry && Entry->Track)
	{
		CrossfadeTo(Entry->Track, Entry->BaseVolume * MusicVolume);
	}

	OnMusicStateChanged.Broadcast(OldState, NewState);
}

void UARPGMusicManager::SetCombatIntensity(float Intensity)
{
	CombatIntensity = FMath::Clamp(Intensity, 0.f, 1.f);

	if (CurrentState == EMusicState::Combat && PrimaryTrack && PrimaryTrack->IsPlaying())
	{
		const FMusicTrackEntry* Entry = TrackMap.Find(EMusicState::Combat);
		const float BaseVol = Entry ? Entry->BaseVolume : 1.f;
		const float IntensityVolume = FMath::Lerp(0.6f, 1.f, CombatIntensity);
		PrimaryTrack->SetVolumeMultiplier(BaseVol * MusicVolume * IntensityVolume);
	}
}

void UARPGMusicManager::RegisterTrack(EMusicState State, USoundBase* Track, float BaseVolume)
{
	FMusicTrackEntry Entry;
	Entry.State = State;
	Entry.Track = Track;
	Entry.BaseVolume = FMath::Clamp(BaseVolume, 0.f, 2.f);
	TrackMap.Add(State, Entry);
}

void UARPGMusicManager::StopMusic(float FadeOutTime)
{
	if (PrimaryTrack && PrimaryTrack->IsPlaying())
	{
		PrimaryTrack->FadeOut(FadeOutTime, 0.f);
	}
	if (SecondaryTrack && SecondaryTrack->IsPlaying())
	{
		SecondaryTrack->FadeOut(FadeOutTime, 0.f);
	}
	CurrentState = EMusicState::None;
}

void UARPGMusicManager::SetMusicVolume(float Volume)
{
	MusicVolume = FMath::Clamp(Volume, 0.f, 1.f);

	if (PrimaryTrack && PrimaryTrack->IsPlaying())
	{
		const FMusicTrackEntry* Entry = TrackMap.Find(CurrentState);
		const float BaseVol = Entry ? Entry->BaseVolume : 1.f;
		PrimaryTrack->SetVolumeMultiplier(BaseVol * MusicVolume);
	}
}

void UARPGMusicManager::CrossfadeTo(USoundBase* NewTrack, float TargetVolume)
{
	if (!NewTrack)
	{
		return;
	}

	EnsureAudioComponentsReady();
	if (!PrimaryTrack || !SecondaryTrack)
	{
		return;
	}

	// Fade out current primary
	if (PrimaryTrack->IsPlaying())
	{
		PrimaryTrack->FadeOut(CrossfadeDuration, 0.f);
	}

	// Swap so the new track plays on Primary
	SwapTracks();

	PrimaryTrack->SetSound(NewTrack);
	PrimaryTrack->SetVolumeMultiplier(0.f);
	PrimaryTrack->Play();
	PrimaryTrack->FadeIn(CrossfadeDuration, TargetVolume);
}

void UARPGMusicManager::SwapTracks()
{
	UAudioComponent* Temp = PrimaryTrack;
	PrimaryTrack = SecondaryTrack;
	SecondaryTrack = Temp;
}

void UARPGMusicManager::EnsureAudioComponentsReady()
{
	if (PrimaryTrack && SecondaryTrack)
	{
		return;
	}

	UWorld* World = GetGameInstance()->GetWorld();
	if (!World)
	{
		return;
	}

	auto CreateMusicComp = [&](const TCHAR* Name) -> UAudioComponent*
	{
		UAudioComponent* Comp = NewObject<UAudioComponent>(World->GetWorldSettings(), Name);
		Comp->bAutoActivate = false;
		Comp->bAutoDestroy = false;
		Comp->bIsUISound = true;
		Comp->RegisterComponentWithWorld(World);
		return Comp;
	};

	if (!PrimaryTrack)
	{
		PrimaryTrack = CreateMusicComp(TEXT("PrimaryMusicTrack"));
	}
	if (!SecondaryTrack)
	{
		SecondaryTrack = CreateMusicComp(TEXT("SecondaryMusicTrack"));
	}
}

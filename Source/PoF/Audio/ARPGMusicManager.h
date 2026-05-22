#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ARPGMusicManager.generated.h"

class UAudioComponent;
class USoundBase;

UENUM(BlueprintType)
enum class EMusicState : uint8
{
	None,
	Exploration,
	Combat,
	Boss,
	Stealth,
	Menu,
	Cinematic
};

USTRUCT(BlueprintType)
struct FMusicTrackEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EMusicState State = EMusicState::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USoundBase> Track;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float BaseVolume = 1.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMusicStateChanged, EMusicState, OldState, EMusicState, NewState);

/**
 * Dynamic music system that crossfades between tracks based on gameplay state.
 * Supports intensity layering and smooth transitions.
 */
UCLASS()
class POF_API UARPGMusicManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void SetMusicState(EMusicState NewState);

	UFUNCTION(BlueprintPure, Category = "Audio|Music")
	EMusicState GetCurrentMusicState() const { return CurrentState; }

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void SetCombatIntensity(float Intensity);

	UFUNCTION(BlueprintPure, Category = "Audio|Music")
	float GetCombatIntensity() const { return CombatIntensity; }

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void RegisterTrack(EMusicState State, USoundBase* Track, float BaseVolume = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void SetCrossfadeDuration(float Duration) { CrossfadeDuration = FMath::Max(Duration, 0.1f); }

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void StopMusic(float FadeOutTime = 2.f);

	UFUNCTION(BlueprintCallable, Category = "Audio|Music")
	void SetMusicVolume(float Volume);

	UPROPERTY(BlueprintAssignable, Category = "Audio|Music")
	FOnMusicStateChanged OnMusicStateChanged;

private:
	UPROPERTY()
	TObjectPtr<UAudioComponent> PrimaryTrack;

	UPROPERTY()
	TObjectPtr<UAudioComponent> SecondaryTrack;

	UPROPERTY()
	TMap<EMusicState, FMusicTrackEntry> TrackMap;

	EMusicState CurrentState = EMusicState::None;
	float CrossfadeDuration = 2.f;
	float CombatIntensity = 0.f;
	float MusicVolume = 0.8f;

	void CrossfadeTo(USoundBase* NewTrack, float TargetVolume);
	void SwapTracks();
	void EnsureAudioComponentsReady();
};

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "ARPGSoundManager.generated.h"

class UAudioComponent;
class USoundBase;
class USoundConcurrency;

UENUM(BlueprintType)
enum class EARPGSoundCategory : uint8
{
	Master,
	Music,
	SFX,
	Ambient,
	UI,
	Voice
};

USTRUCT(BlueprintType)
struct FARPGSoundPoolEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAudioComponent> AudioComponent = nullptr;

	UPROPERTY()
	float LastUsedTime = 0.f;

	bool IsAvailable(const UWorld* World) const;
};

/**
 * Centralized audio manager subsystem.
 * Handles sound playback, volume control per category, sound pooling,
 * and concurrency management.
 */
UCLASS()
class POF_API UARPGSoundManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- Volume Control ---

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetCategoryVolume(EARPGSoundCategory Category, float Volume);

	UFUNCTION(BlueprintPure, Category = "Audio")
	float GetCategoryVolume(EARPGSoundCategory Category) const;

	// --- Sound Playback ---

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (WorldContext = "WorldContextObject"))
	UAudioComponent* PlaySound2D(UObject* WorldContextObject, USoundBase* Sound, EARPGSoundCategory Category = EARPGSoundCategory::SFX, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (WorldContext = "WorldContextObject"))
	UAudioComponent* PlaySoundAtLocation(UObject* WorldContextObject, USoundBase* Sound, FVector Location, EARPGSoundCategory Category = EARPGSoundCategory::SFX, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Audio")
	UAudioComponent* PlaySoundAttached(USoundBase* Sound, USceneComponent* AttachTo, FName SocketName = NAME_None, EARPGSoundCategory Category = EARPGSoundCategory::SFX, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f);

	// --- Sound Pooling ---

	UFUNCTION(BlueprintCallable, Category = "Audio", meta = (WorldContext = "WorldContextObject"))
	UAudioComponent* GetPooledComponent(UObject* WorldContextObject);

	void ReturnToPool(UAudioComponent* Component);

	// --- Persistence ---

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SaveVolumeSettings();

	UFUNCTION(BlueprintCallable, Category = "Audio")
	void LoadVolumeSettings();

	// --- Concurrency ---

	UFUNCTION(BlueprintPure, Category = "Audio")
	USoundConcurrency* GetDefaultConcurrency() const { return DefaultConcurrency; }

private:
	UPROPERTY()
	TMap<EARPGSoundCategory, float> CategoryVolumes;

	UPROPERTY()
	TArray<FARPGSoundPoolEntry> SoundPool;

	UPROPERTY()
	TObjectPtr<USoundConcurrency> DefaultConcurrency;

	static constexpr int32 MaxPoolSize = 32;
	static constexpr float PoolRecycleTime = 0.5f;

	float GetEffectiveVolume(EARPGSoundCategory Category, float Multiplier) const;
	void InitializeDefaultVolumes();
};

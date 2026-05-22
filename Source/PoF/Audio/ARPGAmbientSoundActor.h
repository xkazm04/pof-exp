#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGAmbientSoundActor.generated.h"

class UAudioComponent;
class USoundBase;
class USphereComponent;

UENUM(BlueprintType)
enum class EAmbientSoundType : uint8
{
	Looping,
	RandomInterval,
	TimeOfDay
};

/**
 * Ambient sound emitter with built-in attenuation, occlusion, and spatialization.
 * Supports looping, random-interval triggering, and time-of-day awareness.
 */
UCLASS(Blueprintable)
class POF_API AARPGAmbientSoundActor : public AActor
{
	GENERATED_BODY()

public:
	AARPGAmbientSoundActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Audio|Ambient")
	void SetEnabled(bool bEnable);

	UFUNCTION(BlueprintCallable, Category = "Audio|Ambient")
	void SetVolumeMultiplier(float NewVolume);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAudioComponent> AudioComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> AttenuationSphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient Sound")
	TObjectPtr<USoundBase> AmbientSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient Sound")
	EAmbientSoundType SoundType = EAmbientSoundType::Looping;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient Sound", meta = (ClampMin = "0.0"))
	float AttenuationRadius = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient Sound", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float VolumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient Sound", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float PitchMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient Sound|Random Interval", meta = (EditCondition = "SoundType == EAmbientSoundType::RandomInterval"))
	float MinInterval = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient Sound|Random Interval", meta = (EditCondition = "SoundType == EAmbientSoundType::RandomInterval"))
	float MaxInterval = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient Sound|Random Interval", meta = (EditCondition = "SoundType == EAmbientSoundType::RandomInterval"))
	float RandomPitchVariation = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient Sound")
	bool bEnableOcclusion = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient Sound")
	bool bStartEnabled = true;

	// Time-of-day settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient Sound|Time Of Day", meta = (EditCondition = "SoundType == EAmbientSoundType::TimeOfDay", ClampMin = "0.0", ClampMax = "24.0"))
	float ActiveStartHour = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient Sound|Time Of Day", meta = (EditCondition = "SoundType == EAmbientSoundType::TimeOfDay", ClampMin = "0.0", ClampMax = "24.0"))
	float ActiveEndHour = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient Sound|Time Of Day", meta = (EditCondition = "SoundType == EAmbientSoundType::TimeOfDay", ClampMin = "0.0", ClampMax = "5.0"))
	float TimeOfDayFadeDuration = 2.f;

private:
	float NextTriggerTime = 0.f;
	bool bIsEnabled = true;
	bool bTimeOfDayActive = false;

	void PlayOneShot();
	void ScheduleNextTrigger();
	bool IsWithinActiveHours() const;
};

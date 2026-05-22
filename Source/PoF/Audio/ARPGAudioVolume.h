#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ARPGAudioVolume.generated.h"

class UBoxComponent;
class UAudioComponent;
class UReverbEffect;
class AReverbVolume;

UENUM(BlueprintType)
enum class EReverbPreset : uint8
{
	None,
	SmallRoom,
	LargeHall,
	Cave,
	Forest,
	Underwater,
	Cathedral,
	Arena,
	Custom
};

USTRUCT(BlueprintType)
struct FARPGReverbSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Volume = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.1", ClampMax = "20.0"))
	float DecayTime = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float Density = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float Diffusion = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AirAbsorptionGainHF = 0.99f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "10.0"))
	float DecayHFRatio = 0.83f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReflectionsGain = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LateReverbGain = 1.26f;
};

/**
 * Audio volume that applies reverb and ambient sound when the player enters.
 * Supports preset-based and custom reverb settings with smooth interpolation.
 */
UCLASS(Blueprintable)
class POF_API AARPGAudioVolume : public AActor
{
	GENERATED_BODY()

public:
	AARPGAudioVolume();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Audio|Volume")
	EReverbPreset GetReverbPreset() const { return ReverbPreset; }

	UFUNCTION(BlueprintCallable, Category = "Audio|Volume")
	void SetReverbPreset(EReverbPreset NewPreset);

	UFUNCTION(BlueprintPure, Category = "Audio|Volume")
	const FARPGReverbSettings& GetReverbSettings() const { return ReverbSettings; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> VolumeBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAudioComponent> AmbientAudioComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Volume")
	EReverbPreset ReverbPreset = EReverbPreset::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Volume", meta = (EditCondition = "ReverbPreset == EReverbPreset::Custom"))
	FARPGReverbSettings ReverbSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Volume")
	TObjectPtr<USoundBase> InteriorAmbientSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Volume", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float InteriorAmbientVolume = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Volume")
	FVector VolumeExtent = FVector(500.f, 500.f, 300.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Volume", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Priority = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio Volume", meta = (ClampMin = "0.0", ClampMax = "5.0"))
	float FadeTime = 1.f;

	UFUNCTION()
	void OnVolumeBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnVolumeEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	UPROPERTY()
	TObjectPtr<UReverbEffect> ActiveReverbEffect;

	int32 OverlappingPlayerCount = 0;

	void ApplyReverbPreset(EReverbPreset Preset);
	void ActivateReverb();
	void DeactivateReverb();
	static FARPGReverbSettings GetPresetSettings(EReverbPreset Preset);
};

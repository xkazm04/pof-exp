#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Sound/SoundConcurrency.h"
#include "ARPGSoundConcurrencyConfig.generated.h"

USTRUCT(BlueprintType)
struct FARPGConcurrencyPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName PresetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1", ClampMax = "64"))
	int32 MaxCount = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EMaxConcurrentResolutionRule::Type> ResolutionRule = EMaxConcurrentResolutionRule::StopLowestPriority;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bLimitToOwner = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConcurrencyVolumeScaleMode VolumeScaleMode = EConcurrencyVolumeScaleMode::Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float VolumeScale = 1.f;
};

/**
 * Data asset containing sound concurrency presets for different audio categories.
 * Provides centralized concurrency configuration for the audio system.
 */
UCLASS(BlueprintType)
class POF_API UARPGSoundConcurrencyConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concurrency")
	FARPGConcurrencyPreset WeaponConcurrency = { TEXT("Weapon"), 4, EMaxConcurrentResolutionRule::StopLowestPriority, true, EConcurrencyVolumeScaleMode::Default, 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concurrency")
	FARPGConcurrencyPreset FootstepConcurrency = { TEXT("Footstep"), 3, EMaxConcurrentResolutionRule::StopOldest, true, EConcurrencyVolumeScaleMode::Default, 0.8f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concurrency")
	FARPGConcurrencyPreset AmbientConcurrency = { TEXT("Ambient"), 16, EMaxConcurrentResolutionRule::StopQuietest, false, EConcurrencyVolumeScaleMode::Default, 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concurrency")
	FARPGConcurrencyPreset UIConcurrency = { TEXT("UI"), 2, EMaxConcurrentResolutionRule::StopOldest, false, EConcurrencyVolumeScaleMode::Default, 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concurrency")
	FARPGConcurrencyPreset AbilityConcurrency = { TEXT("Ability"), 6, EMaxConcurrentResolutionRule::StopLowestPriority, false, EConcurrencyVolumeScaleMode::Default, 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Concurrency")
	FARPGConcurrencyPreset ImpactConcurrency = { TEXT("Impact"), 8, EMaxConcurrentResolutionRule::StopQuietest, false, EConcurrencyVolumeScaleMode::Default, 1.f };

	UFUNCTION(BlueprintPure, Category = "Concurrency")
	const FARPGConcurrencyPreset& GetPreset(FName PresetName) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("SoundConcurrencyConfig"), GetFName());
	}
};

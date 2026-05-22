#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "ARPGCombatAudioComponent.generated.h"

class USoundBase;
class UPhysicalMaterial;

UENUM(BlueprintType)
enum class ECombatSoundType : uint8
{
	WeaponSwing,
	WeaponImpact,
	WeaponBlock,
	AbilityCast,
	AbilityImpact,
	Footstep,
	Dodge,
	Death,
	HitReact
};

USTRUCT(BlueprintType)
struct FCombatSoundEntry : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ECombatSoundType SoundType = ECombatSoundType::WeaponSwing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<USoundBase>> SoundVariations;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float VolumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "0.3"))
	float PitchVariation = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AttachSocket = NAME_None;
};

USTRUCT(BlueprintType)
struct FSurfaceSoundEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EPhysicalSurface> SurfaceType = EPhysicalSurface::SurfaceType_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<USoundBase>> ImpactSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TObjectPtr<USoundBase>> FootstepSounds;
};

/**
 * Component for combat-related audio with surface-aware impact sounds,
 * weapon sounds, and ability SFX. Supports randomized variations and
 * pitch variation for natural-sounding repetition.
 */
UCLASS(ClassGroup = "Audio", meta = (BlueprintSpawnableComponent))
class POF_API UARPGCombatAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGCombatAudioComponent();

	UFUNCTION(BlueprintCallable, Category = "Audio|Combat")
	void PlayCombatSound(ECombatSoundType SoundType);

	UFUNCTION(BlueprintCallable, Category = "Audio|Combat")
	void PlayImpactSoundAtLocation(FVector Location, EPhysicalSurface SurfaceType);

	UFUNCTION(BlueprintCallable, Category = "Audio|Combat")
	void PlayFootstepSound(EPhysicalSurface SurfaceType);

	UFUNCTION(BlueprintCallable, Category = "Audio|Combat")
	void PlaySoundWithVariation(USoundBase* Sound, FVector Location, float BaseVolume = 1.f, float PitchVar = 0.1f);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Audio")
	TArray<FCombatSoundEntry> CombatSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Audio|Surface")
	TArray<FSurfaceSoundEntry> SurfaceSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Audio", meta = (ClampMin = "0.0", ClampMax = "5000.0"))
	float MaxAudibleDistance = 3000.f;

private:
	int32 LastFootstepIndex = -1;

	USoundBase* GetRandomSound(const TArray<TObjectPtr<USoundBase>>& Sounds) const;
	const FSurfaceSoundEntry* FindSurfaceEntry(EPhysicalSurface SurfaceType) const;
};

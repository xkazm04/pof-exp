#pragma once

#include "CoreMinimal.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "ARPGPhysicalMaterial.generated.h"

/** Surface types for impact effects and footstep sounds. */
UENUM(BlueprintType)
enum class EARPGGameSurfaceType : uint8
{
	Default,
	Stone,
	Wood,
	Metal,
	Dirt,
	Grass,
	Water,
	Flesh,
	Ice,
	Sand
};

/**
 * Extended physical material with ARPG-specific surface type for
 * VFX/SFX selection on impacts and footsteps.
 */
UCLASS(BlueprintType)
class POF_API UARPGPhysicalMaterial : public UPhysicalMaterial
{
	GENERATED_BODY()

public:
	UARPGPhysicalMaterial();

	/** Game-specific surface type for selecting impact VFX and footstep sounds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface")
	EARPGGameSurfaceType GameSurfaceType = EARPGGameSurfaceType::Default;

	/** Niagara system spawned on projectile/weapon impact with this surface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface|VFX")
	TObjectPtr<class UNiagaraSystem> ImpactVFX;

	/** Sound played on impact with this surface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface|SFX")
	TObjectPtr<class USoundBase> ImpactSound;

	/** Sound played when a character steps on this surface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface|SFX")
	TObjectPtr<class USoundBase> FootstepSound;

	/** Particle scale multiplier for impacts on this surface. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surface|VFX", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float ImpactVFXScale = 1.0f;
};

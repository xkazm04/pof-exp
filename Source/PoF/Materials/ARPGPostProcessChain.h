#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ARPGPostProcessChain.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * A single entry in the post-process material chain.
 */
USTRUCT(BlueprintType)
struct FARPGPostProcessEntry
{
	GENERATED_BODY()

	/** Display name for this PP effect (e.g., "Vignette", "ScreenDamage"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess")
	FName EffectName;

	/** The post-process material to apply. Must have Material Domain = Post Process. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess")
	TSoftObjectPtr<UMaterialInterface> Material;

	/** Whether this effect is currently active. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess")
	bool bEnabled = true;

	/** Blend weight [0..1]. 0 = invisible, 1 = full strength. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess", meta = (ClampMin = "0", ClampMax = "1"))
	float BlendWeight = 1.f;

	/** Priority for ordering. Higher = applied later (on top). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess")
	int32 Priority = 0;

	/** Whether to render after tonemapping (true) or before (false). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess")
	bool bAfterTonemapping = false;

	/** Runtime MID created from the source material. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicInstance;
};

/**
 * Component that manages a chain of post-process materials via blendables
 * on the owning camera or post-process volume.
 *
 * Supports dynamic enable/disable, blend weight animation, and per-effect
 * scalar/vector parameter overrides. Attach to an actor with a
 * UCameraComponent or APostProcessVolume.
 *
 * Usage:
 *   1. Add entries in the editor or at runtime via AddEffect().
 *   2. Enable/disable effects with SetEffectEnabled().
 *   3. Animate parameters with SetEffectScalar()/SetEffectVector().
 *   4. Blend weight transitions with SetEffectBlendWeight() or AnimateBlendWeight().
 */
UCLASS(ClassGroup = (Rendering), meta = (BlueprintSpawnableComponent))
class POF_API UARPGPostProcessChain : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGPostProcessChain();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ----- Chain Management -----

	/** The post-process effect chain. Configured in editor or at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PostProcess")
	TArray<FARPGPostProcessEntry> EffectChain;

	/**
	 * Add an effect at runtime.
	 * @return Index of the new effect in the chain.
	 */
	UFUNCTION(BlueprintCallable, Category = "PostProcess")
	int32 AddEffect(FName EffectName, UMaterialInterface* PPMaterial, float BlendWeight = 1.f, int32 Priority = 0);

	/** Remove an effect by name. */
	UFUNCTION(BlueprintCallable, Category = "PostProcess")
	void RemoveEffect(FName EffectName);

	// ----- Per-Effect Control -----

	/** Enable or disable a named effect. Disabled effects are removed from blendables. */
	UFUNCTION(BlueprintCallable, Category = "PostProcess")
	void SetEffectEnabled(FName EffectName, bool bEnabled);

	/** Set the blend weight of a named effect. */
	UFUNCTION(BlueprintCallable, Category = "PostProcess")
	void SetEffectBlendWeight(FName EffectName, float Weight);

	/**
	 * Smoothly animate an effect's blend weight to a target over Duration seconds.
	 * If Duration <= 0, sets immediately.
	 */
	UFUNCTION(BlueprintCallable, Category = "PostProcess")
	void AnimateBlendWeight(FName EffectName, float TargetWeight, float Duration = 0.5f);

	/** Set a scalar parameter on a named effect's dynamic material. */
	UFUNCTION(BlueprintCallable, Category = "PostProcess")
	void SetEffectScalar(FName EffectName, FName ParamName, float Value);

	/** Set a vector parameter on a named effect's dynamic material. */
	UFUNCTION(BlueprintCallable, Category = "PostProcess")
	void SetEffectVector(FName EffectName, FName ParamName, FLinearColor Value);

	/** Get the MID for a named effect (for advanced manipulation). */
	UFUNCTION(BlueprintPure, Category = "PostProcess")
	UMaterialInstanceDynamic* GetEffectMID(FName EffectName) const;

	/** Check if any effects are currently active. */
	UFUNCTION(BlueprintPure, Category = "PostProcess")
	bool HasActiveEffects() const;

private:
	/** Tracks blend weight animations. */
	struct FBlendAnimation
	{
		FName EffectName;
		float StartWeight;
		float TargetWeight;
		float Duration;
		float Elapsed;
	};

	TArray<FBlendAnimation> BlendAnimations;

	void InitializeChain();
	void ApplyBlendables();
	void RemoveBlendables();
	FARPGPostProcessEntry* FindEntry(FName EffectName);
	const FARPGPostProcessEntry* FindEntry(FName EffectName) const;
};

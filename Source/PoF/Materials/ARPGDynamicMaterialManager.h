#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ARPGDynamicMaterialManager.generated.h"

class UMaterialInstanceDynamic;
class UMeshComponent;

/**
 * Effect types that can be applied to an actor's materials at runtime.
 */
UENUM(BlueprintType)
enum class EARPGMaterialEffect : uint8
{
	None,
	/** Brief flash on hit (scalar pulse on EmissiveStrength). */
	DamageFlash,
	/** Gradual dissolve from bottom to top (death, spawn, teleport). */
	Dissolve,
	/** Outline/fresnel highlight (interact prompt, targeting). */
	Highlight,
	/** Frozen tint overlay. */
	Freeze,
	/** Burning emissive overlay. */
	Burn
};

/**
 * Tracks a single active material effect with its remaining duration and lerp state.
 */
USTRUCT()
struct FARPGActiveEffect
{
	GENERATED_BODY()

	EARPGMaterialEffect Type = EARPGMaterialEffect::None;
	float Duration = 0.f;
	float Elapsed = 0.f;
	float Intensity = 1.f;

	bool IsExpired() const { return Duration > 0.f && Elapsed >= Duration; }
	float GetAlpha() const { return Duration > 0.f ? FMath::Clamp(Elapsed / Duration, 0.f, 1.f) : 0.f; }
};

/**
 * Component that manages dynamic material instances on an actor's mesh.
 * Provides fire-and-forget material effects (damage flash, dissolve, highlight, etc.).
 *
 * Attach to any actor with a UMeshComponent. On BeginPlay it auto-creates MIDs
 * for each material slot, preserving the original material assignments.
 *
 * Parameter names are configurable so they match whatever master material is in use.
 */
UCLASS(ClassGroup = (Rendering), meta = (BlueprintSpawnableComponent))
class POF_API UARPGDynamicMaterialManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UARPGDynamicMaterialManager();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ----- Effect API -----

	/**
	 * Play a one-shot damage flash effect.
	 * @param FlashColor  The color of the flash (typically red/white).
	 * @param Duration    How long the flash lasts.
	 * @param Intensity   Peak emissive strength.
	 */
	UFUNCTION(BlueprintCallable, Category = "Materials|Effects")
	void PlayDamageFlash(FLinearColor FlashColor = FLinearColor::Red, float Duration = 0.15f, float Intensity = 3.f);

	/**
	 * Start a dissolve effect (e.g., on death or spawn).
	 * @param Duration     Total dissolve time.
	 * @param bReverse     If true, dissolves IN (un-dissolve / materialize).
	 * @param EdgeColor    Color of the dissolve edge band.
	 */
	UFUNCTION(BlueprintCallable, Category = "Materials|Effects")
	void StartDissolve(float Duration = 1.5f, bool bReverse = false, FLinearColor EdgeColor = FLinearColor(1.f, 0.3f, 0.f));

	/**
	 * Enable/disable a highlight effect (interactive outline / fresnel glow).
	 * @param bEnable     Turn on or off.
	 * @param Color       Highlight color.
	 * @param Intensity   Highlight strength.
	 */
	UFUNCTION(BlueprintCallable, Category = "Materials|Effects")
	void SetHighlight(bool bEnable, FLinearColor Color = FLinearColor(0.5f, 0.8f, 1.f), float Intensity = 2.f);

	/**
	 * Apply a status effect tint (freeze, burn, etc.).
	 * @param EffectType  Must be Freeze or Burn.
	 * @param Duration    How long the effect lasts. 0 = indefinite (clear manually).
	 * @param Intensity   Effect strength [0..1].
	 */
	UFUNCTION(BlueprintCallable, Category = "Materials|Effects")
	void ApplyStatusEffect(EARPGMaterialEffect EffectType, float Duration = 0.f, float Intensity = 1.f);

	/** Clear all active material effects and reset to base state. */
	UFUNCTION(BlueprintCallable, Category = "Materials|Effects")
	void ClearAllEffects();

	/** Clear a specific effect type. */
	UFUNCTION(BlueprintCallable, Category = "Materials|Effects")
	void ClearEffect(EARPGMaterialEffect EffectType);

	/** Whether the MIDs have been successfully created. */
	UFUNCTION(BlueprintPure, Category = "Materials")
	bool HasDynamicMaterials() const { return DynamicMaterials.Num() > 0; }

	// ----- Parameter name overrides (match your master material) -----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Parameters")
	FName DamageFlashColorParam = TEXT("DamageFlashColor");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Parameters")
	FName DamageFlashIntensityParam = TEXT("DamageFlashIntensity");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Parameters")
	FName DissolveAmountParam = TEXT("DissolveAmount");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Parameters")
	FName DissolveEdgeColorParam = TEXT("DissolveEdgeColor");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Parameters")
	FName HighlightColorParam = TEXT("HighlightColor");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Parameters")
	FName HighlightIntensityParam = TEXT("HighlightIntensity");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Parameters")
	FName FreezeTintParam = TEXT("FreezeTint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|Parameters")
	FName BurnIntensityParam = TEXT("BurnIntensity");

protected:
	/** Mesh component to manage materials on. If null, auto-finds the first UMeshComponent on the owner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
	TObjectPtr<UMeshComponent> TargetMesh;

private:
	/** MIDs created from the mesh's original materials. */
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> DynamicMaterials;

	/** Active effects being processed each tick. */
	TArray<FARPGActiveEffect> ActiveEffects;

	/** State for dissolve direction. */
	bool bDissolveReverse = false;

	/** Whether highlight is currently on (persists until manually cleared). */
	bool bHighlightActive = false;

	void CreateDynamicMaterials();
	void UpdateDamageFlash(float DeltaTime);
	void UpdateDissolve(float DeltaTime);
	void UpdateStatusEffects(float DeltaTime);

	void SetScalarOnAll(FName ParamName, float Value);
	void SetVectorOnAll(FName ParamName, const FLinearColor& Value);

	FARPGActiveEffect* FindEffect(EARPGMaterialEffect Type);
	void RemoveEffect(EARPGMaterialEffect Type);
};

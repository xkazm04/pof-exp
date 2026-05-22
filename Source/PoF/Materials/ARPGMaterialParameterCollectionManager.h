#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ARPGMaterialParameterCollectionManager.generated.h"

class UMaterialParameterCollection;
class UMaterialParameterCollectionInstance;

/**
 * World subsystem that ticks shared Material Parameter Collections.
 *
 * Drives global parameters consumed by all materials in the level:
 *   - Time (accumulated game time)
 *   - Wind (direction + strength for foliage/cloth WPO)
 *   - Global tint (post-apocalyptic color grading overlay, time-of-day shift)
 *   - Wetness (rain/puddle blending factor)
 *
 * Designers assign the MPC asset in project settings or call SetMPC at runtime.
 * The subsystem writes named parameters each frame; materials read them via
 * the MPC node in their graphs.
 */
UCLASS()
class POF_API UARPGMaterialParameterCollectionManager : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

	/**
	 * Set the MPC asset to manage. Call once from a game mode or level blueprint.
	 * @param InMPC  The MPC asset (must be loaded).
	 */
	UFUNCTION(BlueprintCallable, Category = "Materials|MPC")
	void SetMPC(UMaterialParameterCollection* InMPC);

	/** Get the managed MPC instance for this world. */
	UFUNCTION(BlueprintPure, Category = "Materials|MPC")
	UMaterialParameterCollectionInstance* GetMPCInstance() const { return MPCInstance; }

	// ----- Wind -----

	UFUNCTION(BlueprintCallable, Category = "Materials|MPC|Wind")
	void SetWindDirection(FVector Direction);

	UFUNCTION(BlueprintCallable, Category = "Materials|MPC|Wind")
	void SetWindStrength(float Strength);

	UFUNCTION(BlueprintPure, Category = "Materials|MPC|Wind")
	FVector GetWindDirection() const { return WindDirection; }

	UFUNCTION(BlueprintPure, Category = "Materials|MPC|Wind")
	float GetWindStrength() const { return WindStrength; }

	// ----- Global Tint -----

	UFUNCTION(BlueprintCallable, Category = "Materials|MPC|Tint")
	void SetGlobalTint(FLinearColor Tint);

	UFUNCTION(BlueprintPure, Category = "Materials|MPC|Tint")
	FLinearColor GetGlobalTint() const { return GlobalTint; }

	// ----- Wetness -----

	UFUNCTION(BlueprintCallable, Category = "Materials|MPC|Wetness")
	void SetWetness(float InWetness);

	UFUNCTION(BlueprintPure, Category = "Materials|MPC|Wetness")
	float GetWetness() const { return Wetness; }

	// ----- Parameter Names (configurable) -----

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|MPC|Config")
	FName TimeParamName = TEXT("GlobalTime");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|MPC|Config")
	FName WindDirXParamName = TEXT("WindDirectionX");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|MPC|Config")
	FName WindDirYParamName = TEXT("WindDirectionY");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|MPC|Config")
	FName WindStrengthParamName = TEXT("WindStrength");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|MPC|Config")
	FName GlobalTintParamName = TEXT("GlobalTint");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials|MPC|Config")
	FName WetnessParamName = TEXT("Wetness");

private:
	UPROPERTY()
	TObjectPtr<UMaterialParameterCollection> MPC;

	UPROPERTY()
	TObjectPtr<UMaterialParameterCollectionInstance> MPCInstance;

	float AccumulatedTime = 0.f;
	FVector WindDirection = FVector(1.f, 0.f, 0.f);
	float WindStrength = 0.5f;
	FLinearColor GlobalTint = FLinearColor::White;
	float Wetness = 0.f;

	void UpdateMPCParameters();
};

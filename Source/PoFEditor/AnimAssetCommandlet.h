#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "AnimAssetCommandlet.generated.h"

/**
 * Commandlet that programmatically creates animation assets:
 *  - 1D Blend Space (BS1D_Locomotion) — SpeedRatio [0,1] axis
 *  - 2D Blend Space (BS_Locomotion)   — Direction [-180,180] x Speed [0, SprintSpeed]
 *  - Animation Montage shell (AM_MeleeCombo with 3 sections)
 *  - Dodge montage shells (AM_Dodge_Forward, etc.)
 *
 * Run via:
 *   UnrealEditor-Cmd.exe PoF.uproject -run=AnimAsset
 *
 * Optional parameters:
 *   -SpeedMax=900        Override the absolute speed axis max (default: 900 matching SprintSpeed)
 *   -UseSpeedRatio       Use [0,1] SpeedRatio axis instead of absolute speed for BS1D
 */
UCLASS()
class UAnimAssetCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UAnimAssetCommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	/** Creates a 1D blend space with SpeedRatio [0,1] axis (3 samples: Idle/Walk/Sprint). */
	bool CreateBlendSpace1D(const FString& PackagePath, const FString& AssetName, float SpeedMax, bool bUseSpeedRatio);

	/** Creates a 2D blend space with Direction [-180,180] x Speed [0, SpeedMax] axes for strafing. */
	bool CreateBlendSpace2D(const FString& PackagePath, const FString& AssetName, float SpeedMax);

	bool CreateMontageShell(const FString& PackagePath, const FString& AssetName, const TArray<FName>& SectionNames);

	/** Creates a Curve Table with an XP-per-level row (exponential scaling, Levels 1–50). */
	bool CreateXPCurveTable(const FString& PackagePath, const FString& AssetName);

	bool SaveAsset(UObject* Asset, UPackage* Package, const FString& PackagePath);

	/** Parse a float value from commandlet params (e.g., -SpeedMax=900). */
	static bool ParseFloatParam(const FString& Params, const FString& ParamName, float& OutValue);
};
